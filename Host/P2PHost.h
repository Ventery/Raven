#ifndef HOST_P2PHOST_H
#define HOST_P2PHOST_H

#include "../Raven/P2PClientBase.h"

namespace Raven
{
    class P2PHost : public P2PClientBase
    {
    public:
        P2PHost() = delete;
        P2PHost(int localPort, std::string serverIp, int serverPort, EndPointType type);
        ~P2PHost();

        virtual void init();
        virtual void run();
        static bool isContinuous() { return isContinuous_; };

    protected:
        virtual void signalHandler(int sig);
        virtual void handleSignal();
        virtual void handleRead();
        virtual void handleWrite();
        virtual void handleWriteRemains();
        virtual void removeFdFromSet(int fd);

        //new virtual
        virtual void handleBashWriteRemains();
        virtual void handleBashRead();
        virtual void handleBashWrite();

    private:
        void handleStop();
        int masterFd_;
        int slaveFd_;
        int bashPid_;
        bool isBashRunning_;
        static bool isContinuous_;
        fd_set oriReadSet_, readSet_;
        fd_set oriWriteSet_, writeSet_;
        char buff_[MAX_BUFF + 1];
        std::string MessageToBash_;
        std::string newMessageToBash_;
        int fdNum_;
    };
} // namespace Raven

#endif //HOST_P2PHOST_H