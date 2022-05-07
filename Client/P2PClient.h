// Role : Client class.
// Author : Ventery

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

        virtual void run();
        virtual void signalHandler(int sig);
        template <typename T>
        void init(T &t);

    protected:
        virtual void handleSignal();
        virtual void handleRead();
        virtual void handleWrite();
        virtual void handleWriteRemains();
        virtual void removeFdFromSet(int fd);

        // new virtual
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

    template <typename T>
    void P2PClient::init(T &t)
    {
        runState_ = STATE_GETTING_INFO;
        addSig(SIGWINCH, std::bind(&T::signalHandler, &t, SIGWINCH));
        try
        {
            P2PClientBase::init<T>(t);
        }
        catch(const char *msg)
        {
            throw msg;
        }
        context_ = std::make_shared<HptpContext>(contactFd_, RavenConfigIns.aesKeyToPeer_, false);
        setSocketNodelay(subscriberFd_);

        FD_ZERO(&oriReadSet_);
        FD_ZERO(&oriWriteSet_);
        FD_SET(STDIN_FILENO, &oriReadSet_);
        FD_SET(contactFd_, &oriReadSet_);
        FD_SET(subscriberFd_, &oriReadSet_);
        FD_SET(winSubscriberFd_, &oriReadSet_);
        FD_SET(fileTransferFd_, &oriReadSet_);
    }
} // namespace Raven

#endif // CLIENT_P2PCLIENT_H