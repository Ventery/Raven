#include "P2PClientBase.h"

namespace Raven
{
    P2PClientBase::P2PClientBase(int localPort, std::string serverIp, int serverPort) : ClientBase(localPort, serverIp, serverPort)
    {
    }

    void P2PClientBase::init()
    {
        ClientBase::init();        
        Global::PeerInfo peerInfo = getPeerInfo(localPort_, true);
        formatTime();
        std::cout << "Peer :" << peerInfo.ip << " " << peerInfo.port << std::endl;

        int fdToPeer = socketReUsePort(localPort_);
        if (!noBlockConnect(fdToPeer, peerInfo, RavenConfigIns.connectTimeout_))//use transfer
        {
			formatTime("Connect to Host failed.Now use server as transfer...\n");
            close(fdToPeer);
            setNoBlocking(peerInfo.sockToServer);
            contactFd_ = peerInfo.sockToServer;
            useTransfer = true;
        }
        else //use P2P
        {
            formatTime("Connect to Host Success!(use P2P)\n");
            close(peerInfo.sockToServer);
            contactFd_ = fdToPeer;
            useTransfer = false;
        }
    }

    P2PClientBase::~P2PClientBase()
    {
        close(contactFd_);
    }
} // namespace Raven
