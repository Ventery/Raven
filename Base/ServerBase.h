//Role:Server base class.
//Author:Ventery
#ifndef BASE_SERVERBASE_H
#define BASE_SERVERBASE_H
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "EventHandlerBase.h"
#include "Global.h"
#include "noncopyable.h"

namespace Global
{
    class ServerBase : public EventHandlerBase
    {
    public:
        ServerBase() = delete;
        ServerBase(int listenPort);
        ~ServerBase();
        virtual void init();
        virtual void run() = 0;

    protected:
        virtual void signalHandler(int sig);
        virtual void handleNewConnection() = 0;
        virtual void handleSignal() = 0;
        virtual void handleRead(int fd) = 0;
        virtual void handleWrite(int fd) = 0;
        virtual void handleClose() = 0;

        int listenPort_;
        int listenFd;
        bool isRunning_;
        int pipeFds_[2];
        int publisherFd_;
        int subscriberFd_;
        int epollFd_;

        struct sockaddr_in clientAddress_;
        static socklen_t clientAddrlength_;

        epoll_event events_[MAX_EVENT_NUMBER];
    };
}

#endif //BASE_SERVERBASE_H