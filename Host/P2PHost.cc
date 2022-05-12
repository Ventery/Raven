#include "P2PHost.h"

namespace Raven
{
    bool P2PHost::isContinuous_ = true;
    P2PHost::P2PHost(int localPort, std::string serverIp, int serverPort, EndPointType type) : P2PClientBase(localPort, serverIp, serverPort, type), masterFd_(posix_openpt(O_RDWR))
    {
        runState_ = STATE_BEGIN;
        setSocketFD_CLOEXEC(masterFd_);
        grantpt(masterFd_);
        unlockpt(masterFd_);
        setNoBlocking(masterFd_);
        fdNum_ = 20;
    }

    P2PHost::~P2PHost()
    {
        int status;
        wait(&status);
        if (WIFEXITED(status))
        {
            formatTime("Bash returns normally :");
            std::cout << WEXITSTATUS(status) << std::endl;
        }
        else
        {
            formatTime("Bash returns abnormally :");
            std::cout << WEXITSTATUS(status) << std::endl;
        }
        close(slaveFd_);
        close(masterFd_);
        close(contactFd_);
    }

    void P2PHost::run()
    {
        runState_ = STATE_PROCESSING;
        formatTime("Running...\n");
        isBashRunning_ = true;
        isRunning_ = true;
        addAlarm(contactFd_);

        while (isRunning_)
        {
            readSet_ = oriReadSet_;
            writeSet_ = oriWriteSet_;
            int rs = select(fdNum_, &readSet_, &writeSet_, 0, 0);
            if (rs < 0)
            {
                if (errno == EINTR)
                    continue;
                isRunning_ = false;
                formatTime("select() error! : ");
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
                newMessageToBash_.clear();

                if (FD_ISSET(contactFd_, &readSet_))
                {
                    handleRead();
                }
                if (FD_ISSET(masterFd_, &readSet_))
                {
                    handleBashRead();
                }
                if (FD_ISSET(fileTransferFd_, &readSet_))
                {
                    handleNewFileTransFd();
                }
                for (auto it : mapFd2FileTransFerInfo_)
                {
                    if (FD_ISSET(it.first, &readSet_))
                    {
                        handleReadFileTransfer(it.second);
                    }
                }
                if (!newMessage_.empty())
                {
                    handleWrite();
                }
                if (FD_ISSET(contactFd_, &writeSet_))
                {
                    handleWriteRemains();
                }
                if (!newMessageToBash_.empty())
                {
                    handleBashWrite();
                }
                if (FD_ISSET(masterFd_, &writeSet_))
                {
                    handleBashWriteRemains();
                }
            }
        }

        delAlarm(contactFd_);
        handleStop();
    }

    void P2PHost::handleWriteRemains()
    {
        context_->writeNoBlock();
        if (context_->isWriteBufferEmpty())
        {
            FD_CLR(contactFd_, &oriWriteSet_);
        }
    }

    void P2PHost::handleBashWriteRemains()
    {
        writen(masterFd_, MessageToBash_);
        if (MessageToBash_.empty())
        {
            FD_CLR(masterFd_, &oriWriteSet_);
        }
    }

    void P2PHost::handleSignal()
    {
        int ret = read(subscriberFd_, buff_, MAX_BUFF);
        if (ret <= 0)
        {
            return;
        }
        else
        {
            for (int i = 0; i < ret; ++i)
            {
                formatTime("Received signal : ");
                switch (buff_[i])
                {
                case SIGTERM:
                case SIGINT:
                case SIGHUP:
                    std::cout << "Terminated signal" << std::endl;
                    isRunning_ = false;
                    isContinuous_ = false;
                    continue;
                case SIGCHLD:
                    std::cout << "SIGCHLD" << std::endl;
                    isRunning_ = false;
                    isBashRunning_ = false;
                    continue;
                default:
                    std::cout << "Unknown signal : " << buff_[i] << std::endl;
                }
            }
        }
    }

    void P2PHost::handleRead()
    {
        bool zero = false;
        int readNum = context_->readNoBlock(zero);
        if (readNum < 0 || (zero && readNum == 0)) // something wrong
        {
            isRunning_ = false;
            formatTime("connection error! Prepare to reconnect!\n");
        }
        // select is Level Triggered.
        while (!context_->isReadBufferEmpty())
        {
            MessageState state = context_->parseMessage();

            if (state >= PARSE_ERROR_PROTOCOL)
            {
                isRunning_ = false;
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
                std::string tempMsg;
                tempMsg = context_->getText();
                if (context_->getCurrentTextType() == PLAINTEXT_WINCTL)
                {
                    struct winsize size;
                    size.ws_row = stoi(context_->getValueByKey("Row"));
                    size.ws_col = stoi(context_->getValueByKey("Colomn"));

                    if (ioctl(slaveFd_, TIOCSWINSZ, (char *)&size) < 0)
                    {
                        printf("TIOCGWINSZ error");
                    }
                }
                else
                {
                    newMessageToBash_ += tempMsg;
                }
            }
        }
    }

    void P2PHost::handleBashRead()
    {
        int ret = read(masterFd_, buff_, MAX_BUFF);
        newMessage_ += HptpContext::makeMessage(std::string(buff_, ret), "", "", PLAINTEXT);
    }

    void P2PHost::handleWrite()
    {
        // CIPHERTEXT is the default mode
        std::cout<<"before encode"<<std::endl;
        std::cout<<newMessage_<<std::endl;
        newMessage_ = HptpContext::makeMessage(newMessage_, context_->getAesKey(), generateStr(kBlockSize), CIPHERTEXT);
        std::cout<<"after encode"<<std::endl;
        std::cout<<newMessage_<<std::endl;

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

    void P2PHost::handleBashWrite()
    {
        MessageToBash_ += newMessageToBash_;
        writen(masterFd_, MessageToBash_);
        if (!MessageToBash_.empty())
        {
            FD_SET(masterFd_, &oriWriteSet_);
        }
    }

    void P2PHost::handleStop()
    {
        formatTime("Stopping......\n");
        if (isBashRunning_)
        {
            kill(bashPid_, SIGHUP); // bash has it's own session,so we sent SIGHUP to it.
            std::string msg;
            setNoBlocking(subscriberFd_);
            read(subscriberFd_, buff_, MAX_BUFF); // handle remain signal
        }
        runState_ = STATE_END;
        formatTime("Stopped\n");
    }

    void P2PHost::signalHandler(int sig)
    {
        int saveErrno = errno;
        std::cout << std::endl
                  << "P2PHost::signalHandler : " << sig << std::endl;
        std::cout << "STATE_GETTING_INFO : " << runState_ << std::endl;
        if (runState_ == STATE_GETTING_INFO)
        {
            close(contactFd_);
            close(fileTransferFd_);
            if (unlink(FileTransferSocketPath_.c_str()) == -1)
            {
                perror("Unlink fali");
                std::cout << "File path : " << FileTransferSocketPath_.c_str() << std::endl;
            }
            exit(0);
        }
        if (sig != SIGHUP)
        {
            write(publisherFd_, (char *)&sig, 1);
        }
        if (sig != SIGCHLD && sig != SIGHUP)
        {
            isRunning_ = false;
        }
        errno = saveErrno;
    }

    void P2PHost::removeFileFdFromSet(int fd)
    {
        FD_CLR(fd, &oriReadSet_);
        mapFd2FileTransFerInfo_.erase(fd);
        close(fd);
    }

	void P2PHost::addFileFdToSet(int fd)
	{
		FD_SET(fd, &oriReadSet_);
	}
} // namespace Raven