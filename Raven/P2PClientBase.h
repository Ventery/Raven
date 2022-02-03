//role : P2P client base class.
//Author : Ventery
#ifndef RAVEN_P2PCLIENTBASE_H
#define RAVEN_P2PCLIENTBASE_H

#include <memory>

#include "../Base/ClientBase.h"
#include "HptpContext.h"
#include "RavenConfig.h"
#include "Util.h"

namespace Raven
{
    class P2PClientBase : public Global::ClientBase
    {
    public:
        P2PClientBase() = delete;
        P2PClientBase(int localPort, std::string serverIp, int serverPort);
        ~P2PClientBase();

        virtual void init();
        virtual void run() = 0;
    
    protected:
        virtual void signalHandler(int sig) = 0; 
        virtual void handleSignal() = 0;
        virtual void handleRead() = 0;
        virtual void handleWrite() = 0;
        virtual void handleWriteRemains() = 0;

        bool useTransfer; 
        ProgressState runState_;
    };
} // namespace Raven

#endif // RAVEN_P2PCLIENTBASE_H