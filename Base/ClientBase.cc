#include "Global.h"

#include "ClientBase.h"

namespace Global
{
    ClientBase::ClientBase(int localPort, std::string serverIp, int serverPort) : localPort_(localPort),
                                                                                  serverIp_(serverIp),
                                                                                  serverPort_(serverPort),
                                                                                  isRunning_(false)
    {
        assert(socketpair(PF_UNIX, SOCK_STREAM, 0, pipeFds_) != -1);
        publisherFd_ = pipeFds_[0];
        subscriberFd_ = pipeFds_[1];
    }

    void ClientBase::init()
    {
        Global::printCurrentSystem();
        srand(kTimeSeed);

        setNoBlocking(publisherFd_);
        addSig(SIGTERM, std::bind(&ClientBase::signalHandler, this, SIGTERM));
        addSig(SIGINT, std::bind(&ClientBase::signalHandler, this, SIGINT));
        addSig(SIGHUP, std::bind(&ClientBase::signalHandler, this, SIGHUP));
    }

    ClientBase::~ClientBase()
    {
        resetSig(SIGTERM);
        resetSig(SIGINT);
        resetSig(SIGHUP);
        close(publisherFd_);
        close(subscriberFd_);
    }

} // namespace Global