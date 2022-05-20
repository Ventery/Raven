// role : P2P client base class.
// Author : Ventery
#ifndef RAVEN_P2PCLIENTBASE_H
#define RAVEN_P2PCLIENTBASE_H

#include <fcntl.h>
#include <memory>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <termios.h>

#include "../Base/ClientBase.h"
#include "HptpContext.h"
#include "RavenConfig.h"
#include "Util.h"

namespace Raven
{
    class P2PClientBase : public Global::ClientBase
    {
        struct FileTransFerInfo
        {
            FileTransFerInfo(int fd, std::string fileName, int length) : fd(fd),
                                                                         fileName(fileName),
                                                                         length(length),
                                                                         alreadySentLength(0) {}
            int fd;
            std::string fileName;
            int length;
            int alreadySentLength;
        };

    public:
        P2PClientBase() = delete;
        P2PClientBase(int localPort, std::string serverIp, int serverPort, EndPointType type);
        ~P2PClientBase();

        virtual void run() = 0;
        virtual void signalHandler(int sig) = 0;

    protected:
        virtual void handleSignal() = 0;
        virtual void handleRead() = 0;
        virtual void handleWrite() = 0;
        virtual void handleWriteRemains() = 0;
        virtual void removeFileFdFromSet(int fd) = 0;
        virtual void addFileFdToSet(int fd) = 0;

        // for file trans
        virtual void handleNewFileTransFd();
        virtual void handleReadFileTransfer(std::shared_ptr<FileTransFerInfo> it);
        virtual void handleFileTransferMessage(std::shared_ptr<HptpContext> it);
        void createTransferSocket();

        template <typename T>
        void init(T &t);

        std::string unixSocketPath_;
        bool useTransfer_;
        ProgressState runState_;
        EndPointType endPointType_;
        std::string FileTransferSocketPath_;

        int fileTransferFd_;
        std::shared_ptr<HptpContext> context_;
        char fileBuff_[MAX_FILE_BUFFER];

        std::map<int, std::shared_ptr<FileTransFerInfo>> mapFd2FileTransFerInfo_;
        std::map<int, FILE *> mapIdentify2FilePtr_;

        std::string newMessage_; // new message to contactFd_
    };

    template <typename T>
    void P2PClientBase::init(T &t)
    {
        ClientBase::init<T>(t);
        createTransferSocket();
        formatTime();

        Global::PeerInfo peerInfo = getPeerInfo(localPort_, true, endPointType_);
        formatTime();
        std::cout << "Peer :" << peerInfo.ip << " " << peerInfo.port << std::endl;

        int fdToPeer = socketReUsePort(localPort_);

        close(fdToPeer);
        setNoBlocking(peerInfo.sockToServer);
        contactFd_ = peerInfo.sockToServer;
        useTransfer_ = true;

        /*废弃基于TCP的P2P，直接用以上的中转模式
        if (!noBlockConnect(fdToPeer, peerInfo,
                            RavenConfigIns.connectTimeout_)) // use transfer
        {
            formatTime("Connect to Host failed.Now use server as transfer...\n");
            close(fdToPeer);
            setNoBlocking(peerInfo.sockToServer);
            contactFd_ = peerInfo.sockToServer;
            useTransfer_ = true;
        }
        else // use P2P
        {
            formatTime("Connect to Host Success!(use P2P)\n");
            close(peerInfo.sockToServer);
            contactFd_ = fdToPeer;
            useTransfer_ = false;
        }*/
    }
} // namespace Raven

#endif // RAVEN_P2PCLIENTBASE_H