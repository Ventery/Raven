#include <sys/ioctl.h>
#include <termios.h>

#include "P2PClient.h"
#include "../Raven/Util.h"
#include "../Raven/HptpContext.h"

#define STTY_US "stty raw -echo"
#define STTY_DEF "stty -raw echo"

namespace Raven
{
	P2PClient::P2PClient(int localPort, std::string serverIp, int serverPort, EndPointType type) : P2PClientBase(localPort, serverIp, serverPort, type)
	{
		runState_ = STATE_BEGIN;
		assert(socketpair(PF_UNIX, SOCK_STREAM, 0, winPipeFds_) != -1);
		winPublisherFd_ = winPipeFds_[0];
		winSubscriberFd_ = winPipeFds_[1];
		fdNum_ = 20;
	}

	P2PClient::~P2PClient()
	{
		resetSig(SIGWINCH);
		close(winPublisherFd_);
		close(winSubscriberFd_);
		system(STTY_DEF);
		fflush(stdout);
	}

	void P2PClient::run()
	{
		runState_ = STATE_PROCESSING;
		formatTime("Running...\n");
		addAlarm(contactFd_);
		std::cout << std::flush;
		system(STTY_US);
		isRunning_ = true;
		signalHandler(SIGWINCH); // init win sizes

		while (isRunning_)
		{
			readSet_ = oriReadSet_;
			writeSet_ = oriWriteSet_;
			int rs = select(fdNum_, &readSet_, &writeSet_, 0, 0); // select for cross platform
			if (rs < 0)
			{
				if (errno == EINTR)
				{
					continue;
				}
				system(STTY_DEF);
				isRunning_ = false;
				formatTime("select() error! \n");
				std::cout << errno << std::endl;
			}
			else
			{
				if (FD_ISSET(subscriberFd_, &readSet_))
				{
					handleSignal();
				}
				if (!isRunning_)
				{
					break;
				}
				newMessage_.clear();
				if (FD_ISSET(STDIN_FILENO, &readSet_))
				{
					handleReadWin();
				}
				if (FD_ISSET(winSubscriberFd_, &readSet_))
				{
					handleReadWinCTL();
				}
				if (FD_ISSET(contactFd_, &readSet_))
				{
					handleRead();
				}
				if (FD_ISSET(fileTransferFd_, &readSet_))
				{
					handleNewFileTransFd();
				}
                auto backup = mapFd2FileTransFerInfo_;
				for (auto it : backup)
				{
					if (FD_ISSET(it.first, &readSet_))
					{
						handleReadFileTransfer(it.second);
					}
				}

				//先处理读⬆，再处理写⬇
				if (!newMessage_.empty())
				{
					handleWrite();
				}
				if (FD_ISSET(contactFd_, &writeSet_))
				{
					handleWriteRemains();
				}
			}
		}
		runState_ = STATE_END;
		formatTime("Stopping......\n");
		delAlarm(contactFd_);
	}

	void P2PClient::handleWriteRemains()
	{
		context_->writeNoBlock();
		if (context_->isWriteBufferEmpty())
		{
			FD_CLR(contactFd_, &writeSet_);
		}
	}

	void P2PClient::handleSignal()
	{
		char signals[1024];
		int ret = read(subscriberFd_, signals, sizeof(signals));
		if (ret > 0)
		{
			for (int i = 0; i < ret; ++i)
			{
				system(STTY_DEF);
				formatTime("Received signal : \n");
				switch (signals[i])
				{
				case SIGHUP:
				case SIGTERM:
				case SIGINT:
					isRunning_ = false;
					formatTime("Terminated signal\n");
					continue;
				default:
					std::cout << "Unknown signal : " << signals[i] << std::endl;
					system(STTY_US);
				}
			}
		}
	}

	void P2PClient::handleReadWin()
	{
		int ret = read(STDIN_FILENO, buff_, MAX_BUFF);
		newMessage_ += HptpContext::makeMessage(std::string(buff_, ret), "", "", PLAINTEXT);
	}

	void P2PClient::handleReadWinCTL()
	{
		int ret = read(winSubscriberFd_, buff_, MAX_BUFF);
		buff_[ret] = '\0';
		int row, col;
		sscanf(buff_, "%d%d", &row, &col);
		Dict dict;
		dict["Row"] = std::to_string(row);
		dict["Colomn"] = std::to_string(col);
		newMessage_ += HptpContext::makeMessage("", "", "", PLAINTEXT_WINCTL, dict);
	}

	void P2PClient::handleWrite()
	{
		// CIPHERTEXT is the default mode
		newMessage_ = HptpContext::makeMessage(newMessage_, context_->getAesKey(), generateStr(kBlockSize), CIPHERTEXT);
		if (useTransfer_)
		{
			newMessage_ = HptpContext::makeMessage(newMessage_, "", "", TRANSFER);
		}
		context_->pushToWriteBuff(newMessage_);
		context_->writeNoBlock();
		if (!context_->isWriteBufferEmpty())
		{
			FD_SET(contactFd_, &oriWriteSet_);
		}
	}

	void P2PClient::handleRead()
	{
		bool zero = false;
		int readNum = context_->readNoBlock(zero);

		if (readNum < 0 || (zero && readNum == 0))
		{
			system(STTY_DEF);
			isRunning_ = false;
			std::cout << std::endl;
			formatTime("connection is going to close!\n");
		}

		// select is Level Triggered.
		while (!context_->isReadBufferEmpty())
		{
			MessageState state = context_->parseMessage();
			// std::cout<<state<<std::endl;
			if (state >= PARSE_ERROR_PROTOCOL)
			{
				system(STTY_DEF);
				isRunning_ = false;
				formatTime("Parse error\n");
				break;
			}
			else if (state == PARSE_AGAIN)
			{
				break;
			}
			else if (state == PARSE_SUCCESS_KEEPALIVE)
			{
				continue;
			}
			//(state==PARSE_SUCCESS
			else if (context_->getCurrentTextType() == FILETRANSFER)
			{
				handleFileTransferMessage(context_);
			}
			else
			{
				std::cout << context_->getText();
				fflush(stdout);
			}
		}
	}

	void P2PClient::signalHandler(int sig)
	{
		int saveErrno = errno;
		/*std::cout << std::endl
				  << "P2PClient::signalHandler : " << sig << std::endl;
		std::cout << "STATE_GETTING_INFO : " << runState_ << std::endl;*/
		if (sig != SIGWINCH)
		{
			if (runState_ == STATE_GETTING_INFO)
			{
				close(subscriberFd_);
				close(fileTransferFd_);
				if (unlink(FileTransferSocketPath_.c_str()) == -1)
				{
					perror("Unlink fali");
					std::cout << "File path : " << FileTransferSocketPath_.c_str() << std::endl;
				}
				exit(0);
			}
			write(publisherFd_, (char *)&sig, 1);
			// system(STTY_DEF);
		}
		else // (sig == SIGWINCH)
		{
			struct winsize size;
			if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&size) < 0)
			{
				formatTime("TIOCGWINSZ error\n");
				return;
			}
			std::string message = std::to_string(size.ws_row) + " " + std::to_string(size.ws_col);
			write(winPublisherFd_, message.c_str(), message.length());
		}
		errno = saveErrno;
	}

	void P2PClient::removeFileFdFromSet(int fd)
	{
		FD_CLR(fd, &oriReadSet_);
		mapFd2FileTransFerInfo_.erase(fd);
		close(fd);
	}

	void P2PClient::addFileFdToSet(int fd)
	{
		FD_SET(fd, &oriReadSet_);
	}

} // namepace Raven
