#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include <map>

#include "../Base/ServerBase.h"
#include "../Raven/HptpContext.h"
#include "../Raven/RavenConfig.h"
#include "../Raven/Util.h"

namespace Raven
{
    class P2PServer : public Global::ServerBase
    {
    public:
        P2PServer() = delete;
        P2PServer(int listenPort);
        ~P2PServer();
        virtual void init();
        virtual void run();

    protected:
        virtual void handleNewConnection();
        virtual void handleSignal();
        virtual void handleRead(int fd);
        virtual void handleWrite(int fd);
        virtual void handleClose();
        void resetSock(std::shared_ptr<HptpContext>&);

        std::map<std::string, std::shared_ptr<HptpContext>> mapIdkey2Address_[2];   //mapIdkey2Address_[0] for client and 1 for host.
        std::map<int, std::shared_ptr<HptpContext>> mapSock2Address_;
        std::set<int> socksToClose_;
    };
} // namespace Raven

#endif // SERVER_SERVER_H