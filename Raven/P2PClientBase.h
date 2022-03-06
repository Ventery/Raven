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
        struct FileTransFerInfo
        {
            FileTransFerInfo(int fd,std::string fileName,int length):
                        fd(fd),
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

        virtual void init();
        virtual void run() = 0;

    protected:
        virtual void signalHandler(int sig) = 0;
        virtual void handleSignal() = 0;
        virtual void handleRead() = 0;
        virtual void handleWrite() = 0;
        virtual void handleWriteRemains() = 0;
        virtual void removeFdFromSet(int fd) = 0;

        //for file trans
        virtual void handleNewFileTransFd();
        virtual void handleReadFileTransfer(std::shared_ptr<FileTransFerInfo> it);
        virtual void handleFileTransferMessage(std::shared_ptr<HptpContext> it);
        void createTransferSocket();

        std::string unixSocketPath;
        bool useTransfer;
        ProgressState runState_;
        EndPointType endPointType_;
        std::string FileTransferSocketPath_;

        int fileTransferFd_;
        std::shared_ptr<HptpContext> context_;
        char fileBuff_[MAX_FILE_BUFFER];

        std::map<int, std::shared_ptr<FileTransFerInfo>> mapFd2FileTransFerInfo_;
        std::map<int,FILE *> mapIdentify2FilePtr_;

        std::string newMessage_;  //new message to contactFd_
    };
} // namespace Raven

#endif // RAVEN_P2PCLIENTBASE_H