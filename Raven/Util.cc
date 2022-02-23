#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "Util.h"
#include "RavenConfig.h"
#include "HptpContext.h"

namespace Raven
{
	const std::string kMsgTooMuch = HptpContext::makeMessage("Too much Connection!", "", "", PLAINTEXT);
	const std::string kKeepaliveMessage = HptpContext::makeMessage("", "", "", KEEPALIVE);
	const std::string kMsgPeerClosed = HptpContext::makeMessage("From server : peer closed!\n", "", "", PLAINTEXT);
	std::set<int> keepAliveFds;
	MutexLock m_keepAliveFds;

	void addAlarm(const int &fd)
	{
		if (keepAliveFds.empty())
		{
			struct sigaction sa;
			memset(&sa, '\0', sizeof(sa));
			sa.sa_handler = handleAlarm;
			sa.sa_flags |= SA_RESTART; //SA_RESTART does not work when use epoll,poll and select.
			sigfillset(&sa.sa_mask);
			assert(sigaction(SIGALRM, &sa, NULL) != -1);
			alarm(RavenConfigIns.keepAliveSec_);
		}

		keepAliveFds.insert(fd);
	}

	void delAlarm(const int &fd)
	{
		keepAliveFds.erase(fd);
		if (keepAliveFds.empty())
		{
			struct sigaction sa;
			memset(&sa, '\0', sizeof(sa));
			sa.sa_handler = SIG_DFL;
			sa.sa_flags |= SA_RESTART;
			assert(sigaction(SIGALRM, &sa, NULL) != -1);
			alarm(0);
		}
	}

	void handleAlarm(int sig)
	{
		assert(!keepAliveFds.empty());
		int saveErrno = errno;
		std::string tpStr = kKeepaliveMessage;
		for (const int &fd : keepAliveFds)
		{
			write(fd, tpStr.c_str(), tpStr.size());
		}
		errno = saveErrno;
		alarm(RavenConfigIns.keepAliveSec_);
	}

	PeerInfo getPeerInfo(const int &port, const bool &useCipher, const EndPointType &type)
	{
		struct sockaddr_in serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = inet_addr(RavenConfigIns.serverIp_.c_str());
		serverAddr.sin_port = htons(RavenConfigIns.serverPort_);

		int fd = socketReUsePort(port);
		PeerInfo result;
		formatTime("Connecting to server...\n");

		Dict dic;
		dic["EndPointType"] = std::to_string((int)type);
		dic["IdentifyKey"] = RavenConfigIns.identifyKey_;
		std::string shakeMessage = HptpContext::makeMessage("", "", "", PLAINTEXT, dic);
		int ret = 0;

		while (true)
		{
			if (ret == -1)
			{
				fd = socketReUsePort(port);
			}
			ret = 0;
			assert(fd);
			if (connect(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
			{
				if (errno == EISCONN)
				{
					formatTime();
					perror("Connecting");
				}
				else
				{
					ret = -1;
					formatTime();
					perror("Connect error");
					close(fd);
					sleep(1);
					continue;
				}
			}
			HptpContext context(fd, RavenConfigIns.aesKeyToServer_);
			std::string message;
			if (useCipher)
			{
				message = HptpContext::makeMessage(shakeMessage, context.getAesKey(), generateStr(kBlockSize), CIPHERTEXT);
			}
			else
			{
				message = shakeMessage;
			}
			context.writeBlock(message);
			formatTime("Connected and waiting peer...\n");
			addAlarm(fd);
			while (true)
			{
				int readNum = context.readBlock();
				if (readNum == 0)
				{
					ret = -1;
					break;
				}

				while (!context.isReadBufferEmpty())
				{
					auto code = context.parseMessage();
					if (code == PARSE_AGAIN)
					{
						break;
					}
					else if (code == PARSE_ERROR)
					{
						ret = -1;
						break;
					}
					else if (code == PARSE_SUCCESS)
					{
						ret = 1;
						break;
					}
				}
				if (ret != 0)
				{
					break;
				}
			}
			delAlarm(fd);
			if (ret == -1)
			{
				close(fd);
				continue;
			}

			result.ip = context.getValueByKey("PeerIp");
			result.port = stoi(context.getValueByKey("PeerPort"));
			result.sockToServer = fd;
			break;
		}
		return result;
	}

	bool noBlockConnect(const int &fd, const struct PeerInfo &peer, const int &timeOutSec)
	{
		struct sockaddr_in servAddr;
		servAddr.sin_family = AF_INET;
		servAddr.sin_addr.s_addr = inet_addr(peer.ip.c_str());
		servAddr.sin_port = htons(atoi(peer.port.c_str()));
		socklen_t addressLen = sizeof(servAddr);

		formatTime("Timeout : ");
		std::cout << RavenConfigIns.connectTimeout_ << " seconds" << std::endl;
		formatTime("Connecting... \n");
		setNoBlocking(fd);
		int seconds = timeOutSec;
		while (seconds--)
		{
			if (connect(fd, (struct sockaddr *)&servAddr, addressLen) == -1)
			{
				if (errno == EISCONN)
				{
					return true;
				}
				else
				{
					sleep(1);
					formatTime();
					for (int i = 0; i < timeOutSec - seconds; i++)
					{
						std::cout << ".";
					}
					std::cout << std::endl;
					continue;
				}
			}
			else
			{
				return true;
			}
		}
		return false;
	}

	//Deprecated
	int AsyncConnect(const int &iSocketFd, const int &timeOutSec)
	{
		fd_set reads, tempWrite, tempErr;
		FD_ZERO(&reads);
		FD_SET(iSocketFd, &reads);
		int result;
		struct timeval timeout;

		while (true)
		{
			tempWrite = reads;
			tempErr = reads;
			timeout.tv_sec = timeOutSec;
			timeout.tv_usec = 0;
			result = select(1, 0, &tempWrite, &tempErr, &timeout);
			if (result == -1)
			{
				return -1;
			}
			if (result == 0)
			{
				return 0;
			}
			if (FD_ISSET(iSocketFd, &tempErr))
			{
				return -1;
			}
			if (FD_ISSET(iSocketFd, &tempWrite)) //always not set
			{
				return 1;
			}
		}
	}
/*
	//Deprecated
	pid_t get_tid()
	{
#ifdef __linux__
		return static_cast<pid_t>(syscall(SYS_gettid));
#elif __APPLE__
		uint64_t id;
		pthread_threadid_np(0, &id);
		return static_cast<pid_t>(syscall(SYS_thread_selfid));
#endif
	}
*/

} // namespace Raven
