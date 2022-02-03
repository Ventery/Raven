#include "ServerBase.h"

namespace Global
{
    socklen_t ServerBase::clientAddrlength_ = sizeof(sockaddr_in);
    ServerBase::ServerBase(int listenPort) : listenPort_(listenPort), isRunning_(true)
    {
        formatTime("ServerBase constructor begin\n");
        assert(listenFd = socketBindListen(listenPort_));
        assert(socketpair(PF_UNIX, SOCK_STREAM, 0, pipeFds_) != -1);
        assert(epollFd_ = epoll_create(4396));
        publisherFd_ = pipeFds_[0];
        subscriberFd_ = pipeFds_[1];

        addFd(epollFd_, listenFd);
        addFd(epollFd_, subscriberFd_);
    }

    void ServerBase::init()
    {
        formatTime("ServerBase constructor init\n");
        srand(kTimeSeed);
        setNoBlocking(publisherFd_);
        addSig(SIGTERM, std::bind(&ServerBase::signalHandler, this, SIGTERM));
        addSig(SIGINT, std::bind(&ServerBase::signalHandler, this, SIGINT));
        addSig(SIGHUP, std::bind(&ServerBase::signalHandler, this, SIGHUP));
    }

    ServerBase::~ServerBase()
    {
        formatTime("ServerBase destructor begin\n");
        close(publisherFd_);
        close(subscriberFd_);
        close(listenFd);
        close(epollFd_);
        formatTime("ServerBase destructor end\n");
    }

    void ServerBase::signalHandler(int sig)
    {
        int save_errno = errno;
        int msg = sig;
        write(publisherFd_, (char *)&msg, 1);
        errno = save_errno;
    }
}