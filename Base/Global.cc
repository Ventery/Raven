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
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "Global.h"

namespace Global
{
	const std::string kHomePath = getenv("HOME");
	const std::string kConfigPath = kHomePath + "/conf/Raven.conf";
	const std::string kFileTransferPath = kHomePath + "/RavenTrans/";
	const size_t kKeySize = gcry_cipher_get_algo_keylen(CIPHER_ALGO);
	const size_t kBlockSize = gcry_cipher_get_algo_blklen(CIPHER_ALGO);
	const unsigned kTimeSeed = time(0);

	std::map<int, EventHandlerBase::CallBack> signalHandlerMap;
	MutexLock mSignalHandlerMap;
	MutexLock mGetBash;
	MutexLock mRefreshScreamer;

	std::string encode(const std::string &buf, const std::string &key, const std::string &init_vec)
	{
		std::string message;
		gcry_cipher_hd_t cipherHd;

		size_t fileSize;
		size_t blockRequired;

		fileSize = buf.size();
		blockRequired = fileSize / kBlockSize;
		if (fileSize % kBlockSize != 0)
		{
			blockRequired++;
		}
		char cipherBuffer[blockRequired * kBlockSize];
		memset(cipherBuffer, 0, blockRequired * kBlockSize);

		if (fileSize)
		{
			gcry_cipher_open(&cipherHd, CIPHER_ALGO, GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_CBC_CTS);
			gcry_cipher_setkey(cipherHd, key.c_str(), kKeySize);
			gcry_cipher_setiv(cipherHd, init_vec.c_str(), kBlockSize);

			memcpy(cipherBuffer, buf.c_str(), fileSize);
			gcry_cipher_encrypt(cipherHd, cipherBuffer, blockRequired * kBlockSize, NULL, 0);
			gcry_cipher_close(cipherHd);
		}
		// encode over

		if (fileSize)
		{
			message = std::string(cipherBuffer, blockRequired * kBlockSize);
		}
		return message;
	}

	std::string decode(const std::string &msg, const std::string &key, const std::string &iv, const int &sourceLength)
	{
		unsigned int length = (sourceLength % kBlockSize == 0 ? sourceLength : ((sourceLength) / kBlockSize + 1) * kBlockSize);
		char decodeBuf[length];

		gcry_cipher_hd_t cipherHd;
		gcry_cipher_open(&cipherHd, CIPHER_ALGO, GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_CBC_CTS);
		gcry_cipher_setkey(cipherHd, key.c_str(), kKeySize);
		gcry_cipher_setiv(cipherHd, iv.c_str(), kBlockSize);
		memcpy(decodeBuf, msg.c_str(), msg.size());

		gcry_cipher_decrypt(cipherHd, decodeBuf, length, NULL, 0);
		gcry_cipher_close(cipherHd);
		return std::string(decodeBuf, sourceLength);
	}

#ifdef __linux__
	void addFd(int epollFd, int fd)
	{
		epoll_event event;
		event.data.fd = fd;
		event.events = DEFAULT_EPOLL_EVENT;
		epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
		setNoBlocking(fd);
	}

	void removeFd(int epollfd, int fd)
	{
		epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
	}
#endif

	void setSocketNodelay(int fd)
	{
		int enable = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
	}

	void setSocketFD_CLOEXEC(int fd)
	{
		fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
	}

	ssize_t readn(int fd, std::string &inBuffer, bool &zero)
	{
		ssize_t nRead = 0;
		ssize_t readSum = 0;

		while (true)
		{
			char buff[MAX_BUFF];
			if ((nRead = read(fd, buff, MAX_BUFF)) < 0)
			{
				if (errno == EINTR)
				{
					continue;
				}
				else if (errno == EAGAIN)
				{
					return readSum;
				}
				else
				{
					perror("Connection error");
					return -1;
				}
			}
			else if (nRead == 0)
			{
				zero = true;
				break;
			}
			readSum += nRead;
			inBuffer += std::string(buff, buff + nRead);
		}
		return readSum;
	}

	ssize_t writen(int fd, std::string &sbuff)
	{
		size_t nleft = sbuff.size();
		ssize_t nWritten = 0;
		ssize_t writeSum = 0;
		const char *ptr = sbuff.c_str();

		while (nleft > 0)
		{
			if ((nWritten = write(fd, ptr, nleft)) <= 0)
			{
				if (nWritten < 0)
				{
					if (errno == EINTR)
					{
						nWritten = 0;
						continue;
					}
					else if (errno == EAGAIN)
					{
						break;
					}
					else
					{
						return -1;
					}
				}
			}
			writeSum += nWritten;
			nleft -= nWritten;
			ptr += nWritten;
		}
		if (writeSum == static_cast<int>(sbuff.size()))
		{
			sbuff.clear();
		}
		else
		{
			sbuff = sbuff.substr(writeSum);
		}
		return writeSum;
	}

	int setNoBlocking(int fd)
	{
		int oldOption = fcntl(fd, F_GETFL);
		int newOption = oldOption | O_NONBLOCK;
		fcntl(fd, F_SETFL, newOption);
		return oldOption;
	}

	int setBlocking(int fd)
	{
		int oldOption = fcntl(fd, F_GETFL);
		int newOption = oldOption & (~O_NONBLOCK);
		fcntl(fd, F_SETFL, newOption);
		return oldOption;
	}

	int socketBindListen(int port)
	{
		if (port < 0 || port > 65535)
			return -1;

		int listenFd = 0;
		if ((listenFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			return -1;

		int optVal = 1;
		if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) == -1)
		{
			close(listenFd);
			return -1;
		}

		struct sockaddr_in serverAddr;
		bzero((char *)&serverAddr, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serverAddr.sin_port = htons((unsigned short)port);
		if (::bind(listenFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		{
			close(listenFd);
			return -1;
		}

		if (listen(listenFd, 2048) == -1)
		{
			close(listenFd);
			return -1;
		}

		if (listenFd == -1)
		{
			close(listenFd);
			return -1;
		}
		return listenFd;
	}

	int socketReUsePort(int port)
	{
		if (port < 0 || port > 65535)
			return -1;

		int p2pFd = 0;
		if ((p2pFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			return -1;

		int optval = 1;
		if (setsockopt(p2pFd, SOL_SOCKET, SO_REUSEADDR, &optval,
					   sizeof(optval)) == -1)
		{
			close(p2pFd);
			return -1;
		}
		if (setsockopt(p2pFd, SOL_SOCKET, SO_REUSEPORT, &optval,
					   sizeof(optval)) == -1)
		{
			close(p2pFd);
			return -1;
		}

		struct sockaddr_in serverAddr;
		bzero((char *)&serverAddr, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serverAddr.sin_port = htons((unsigned short)port);
		if (::bind(p2pFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		{
			close(p2pFd);
			return -1;
		}
		return p2pFd;
	}

	void formatTime(const std::string &msg)
	{
		struct timeval tv;
		time_t time;
		char str[26] = {0};
		gettimeofday(&tv, NULL);
		time = tv.tv_sec;
		struct tm *pTime = localtime(&time);
		strftime(str, 26, "[%Y-%m-%d %H:%M:%S]", pTime);
		printf("%s %s", str, msg.c_str());
		fflush(stdout);
	}

	char number2Char(long num)
	// 0~25 a~z
	// 26~51 A~Z
	// 52~61 0~9
	{
		if (num < 26)
		{
			return num + 65;
		}
		else if (num < 52)
		{
			return num + 71;
		}
		return num - 4;
	}

	std::string generateStr(size_t length)
	{
		char tempStr[length];
		for (size_t i = 0; i < length; i++)
		{
			tempStr[i] = number2Char(rand() % 62);
		}
		return std::string(tempStr, length);
	}

	pid_t getBash(const int slaveFd)
	{
		MutexLockGuard gard(mGetBash);
		pid_t pid = fork();
		if (pid != 0)
		{
			return pid;
		}
		setsid(); // new bash is a new session

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		dup(slaveFd);
		dup(slaveFd);
		dup(slaveFd);

		if (execl(BASH_PATH, "bash", "-l", 0) == -1)
		{
			return -2;
		}
		return 0;
	}

	std::string getExePath(int upLevel)
	{
		char buf[MAX_PATH];
		memset(buf, 0, MAX_PATH);

#ifdef __linux__
		int rslt = readlink("/proc/self/exe", buf, MAX_PATH - 1);
		if (rslt < 0 || (rslt >= MAX_PATH - 1))
		{
			return NULL;
		}

#elif __APPLE__
		getcwd(buf, MAX_PATH);
#endif

		upLevel++;
		std::string rs = buf;
		const char *cPtr = rs.c_str();

		int pos = rs.size() - 1;
		while (upLevel)
		{
			if (cPtr[pos] == '/')
				--upLevel;
			--pos;
		}
		++pos;

		return rs.substr(0, pos + 1);
	}

	void printCurrentSystem()
	{
#ifdef __linux__
		printf("Linux system!\n");
#elif __APPLE__
		printf("MacOS system!\n");
#elif __unix__
		printf("Unix system!\n");
		else printf("Unknown system!\n");
#endif
	}

	void addSig(int sig, EventHandlerBase::CallBack &&call)
	{
		MutexLockGuard gard(mSignalHandlerMap);
		signalHandlerMap[sig] = call;
		struct sigaction sa;
		memset(&sa, '\0', sizeof(sa));
		sa.sa_handler = sigHandler;
		sa.sa_flags |= SA_RESTART;
		sigfillset(&sa.sa_mask);
		assert(sigaction(sig, &sa, NULL) != -1);
	}

	void resetSig(int sig)
	{
		MutexLockGuard gard(mSignalHandlerMap);
		signalHandlerMap.erase(sig);
		struct sigaction sa;
		memset(&sa, '\0', sizeof(sa));
		sa.sa_handler = SIG_DFL;
		sa.sa_flags |= SA_RESTART;
		sigfillset(&sa.sa_mask);
		assert(sigaction(sig, &sa, NULL) != -1);
	}

	void sigHandler(int sig)
	{
		if (signalHandlerMap.find(sig) != signalHandlerMap.end())
		{
			signalHandlerMap[sig]();
		}
	}

	void ifDaemon()
	{
#if defined DEBUG_VERSION
		printf("Defined DEBUG_VERSION!\n");
#elif defined RELEASE_VERSION
		printf("Defined RELEASE_VERSION!\n");
		if (daemon(0, 0) == -1)
		{
			perror("Make daemon error!");
			exit(1);
		}
#else
		printf("Nothing Defined!\n");
#endif
	}

} // namespace Global