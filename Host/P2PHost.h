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

        virtual void run();
        virtual void signalHandler(int sig);
        static bool isContinuous() { return isContinuous_; };
        template <typename T>
        void init(T &t);

    protected:
        virtual void handleSignal();
        virtual void handleRead();
        virtual void handleWrite();
        virtual void handleWriteRemains();
        virtual void removeFdFromSet(int fd);

        // new virtual
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

    template <typename T>
    void P2PHost::init(T &t)
    {
        runState_ = STATE_GETTING_INFO;
        addSig(SIGCHLD, std::bind(&T::signalHandler, &t, SIGCHLD));
        P2PClientBase::init<T>(t);

        slaveFd_ = open(ptsname(masterFd_), O_RDWR);
        bashPid_ = getBash(slaveFd_);
        formatTime("bash Pid = ");
        if (bashPid_ < 0)
        {
            throw "Bash start failed";
        }
        std::cout << bashPid_ << std::endl;

        context_ = std::make_shared<HptpContext>(contactFd_, RavenConfigIns.aesKeyToPeer_, true);
        ifDaemon();
        setSocketFD_CLOEXEC(publisherFd_);
        setSocketFD_CLOEXEC(subscriberFd_);
        setSocketFD_CLOEXEC(contactFd_);

        FD_ZERO(&oriReadSet_);
        FD_ZERO(&oriWriteSet_);
        FD_SET(contactFd_, &oriReadSet_);
        FD_SET(masterFd_, &oriReadSet_);
        FD_SET(subscriberFd_, &oriReadSet_);
        FD_SET(fileTransferFd_, &oriReadSet_);
    }
} // namespace Raven

#endif // HOST_P2PHOST_H