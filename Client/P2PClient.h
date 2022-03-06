//Role : Client class.
//Author : Ventery

#ifndef CLIENT_P2PCLIENT_H
#define CLIENT_P2PCLIENT_H

#include "../Raven/P2PClientBase.h"

namespace Raven
{
    class P2PClient : public P2PClientBase
    {
    public:
        P2PClient() = delete;
        P2PClient(int localPort, std::string serverIp, int serverPort, EndPointType type);
        ~P2PClient();

        virtual void init();
        virtual void run();

    protected:
        virtual void signalHandler(int sig);
        virtual void handleSignal();
        virtual void handleRead();
        virtual void handleWrite();
        virtual void handleWriteRemains();
        virtual void removeFdFromSet(int fd);

        //new virtual
        virtual void handleReadWin();
        virtual void handleReadWinCTL();

    private:
        int winPipeFds_[2];
        int winPublisherFd_;
        int winSubscriberFd_;

        fd_set oriReadSet_, readSet_;
        fd_set oriWriteSet_, writeSet_;
        char buff_[MAX_BUFF];
        int fdNum_;

    };
} //namespace Raven

#endif // CLIENT_P2PCLIENT_H