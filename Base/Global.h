//role : Global variables and functions.
//Author : Ventery

#ifndef BASE_GLOBAL_H
#define BASE_GLOBAL_H

#include <assert.h>
#include <functional>
#include <stdio.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/epoll.h>
#endif

#include <iostream>
#include <memory>
#include <map>
#include <set>
#include <signal.h>

#include <gcrypt.h>
#include <libgen.h>

#include "ClientBase.h"
#include "MutexLock.h"
#include "ReadConfig.h"
#include "Singleton.h"

#define CIPHER_ALGO GCRY_CIPHER_AES128 //使用AES128进行加密
#define BASH_PATH "/bin/bash"

//NETWORK
#define MAX_EVENT_NUMBER 1024
#define MAX_BUFF 1024 * 100
#define MAX_CONNECT_NUMBER 2048
#define MAX_PATH 1024
#ifdef __linux__
#define DEFAULT_EPOLL_EVENT EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLET
#else
#define DEFAULT_EPOLL_EVENT 0
#endif

#define ConfigIns Global::Config::GetInstance()
namespace Global
{
    //Definiton

    enum EndPointType
    {
        TYPE_CLIENT = 1,
        TYPE_HOST,
        TYPE_SERVER,
    };

    struct PeerInfo
    {
        int sockToServer;
        std::string ip;
        std::string port;
    };

    //Var
    //const var
    extern const std::string kHomePath;
    extern const std::string kConfigPath;
    extern const size_t kKeySize;
    extern const size_t kBlockSize;
    extern const unsigned kTimeSeed;

    //normal var
    extern std::map<int, EventHandlerBase::CallBack> signalHandlerMap;
    extern MutexLock mSignalHandlerMap;
    extern MutexLock mGetBash;

    //Func
    //aes
    std::string encode(const std::string &, const std::string &, const std::string &);
    std::string decode(const std::string &, const std::string &, const std::string &, const int &);

    //socket
#ifdef __linux__
    void addFd(int epollfd, int fd);
    void removeFd(int epollfd, int fd);
#endif
    void setSocketNodelay(int fd);
    void setSocketFD_CLOEXEC(int fd);

    //io
    ssize_t readn(int fd, std::string &inBuffer, bool &zero);
    ssize_t writen(int fd, std::string &outBuff);
    int setNoBlocking(int fd);
    int setBlocking(int fd);
    int socketBindListen(int port);
    int socketReUsePort(int port);

    //system
    void printCurrentSystem();
    void addSig(int sig, EventHandlerBase::CallBack &&call);
    void resetSig(int sig);
    void sigHandler(int sig);
    void ifDaemon();

    //others
    pid_t getTid();
    std::string getExePath(int upLevel = 0);
    void formatTime(const std::string & = "");
    pid_t getBash(const int slaveFd);

    //tools
    char number2Char(long num);
    std::string generateStr(size_t length);
    void signalCallFunction(int sig);
} // namespace Global

#endif // BASE_GLOBAL_H
