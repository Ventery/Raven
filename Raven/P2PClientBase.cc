#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "P2PClientBase.h"
namespace Raven
{
    P2PClientBase::P2PClientBase(int localPort,
                                 std::string serverIp,
                                 int serverPort,
                                 EndPointType type)
        : ClientBase(localPort, serverIp, serverPort), endPointType_(type) {}

    void P2PClientBase::init()
    {
        ClientBase::init();
        createTransferSocket();
        FileTransferSocketPath_ = generateStr(8);
        Global::PeerInfo peerInfo = getPeerInfo(localPort_, true, endPointType_);
        formatTime();
        std::cout << "Peer :" << peerInfo.ip << " " << peerInfo.port << std::endl;

        int fdToPeer = socketReUsePort(localPort_);
        if (!noBlockConnect(fdToPeer, peerInfo,
                            RavenConfigIns.connectTimeout_)) // use transfer
        {
            formatTime("Connect to Host failed.Now use server as transfer...\n");
            close(fdToPeer);
            setNoBlocking(peerInfo.sockToServer);
            contactFd_ = peerInfo.sockToServer;
            useTransfer = true;
        }
        else // use P2P
        {
            formatTime("Connect to Host Success!(use P2P)\n");
            close(peerInfo.sockToServer);
            contactFd_ = fdToPeer;
            useTransfer = false;
        }
    }

    void P2PClientBase::createTransferSocket()
    {
        FileTransferSocketPath_ = kFileTransferPath + generateStr(8) + "server.socket";
        std::cout << "FileTransferSocket :" << FileTransferSocketPath_ << std::endl;

        struct sockaddr_un serverConfig;
        int fileTransferFd_;

        if ((fileTransferFd_ = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        {
            throw "Error occurred while getting file Socket fd!";
        }

        memset(&serverConfig, 0, sizeof(serverConfig));
        serverConfig.sun_family = AF_UNIX;
        strcpy(serverConfig.sun_path, FileTransferSocketPath_.c_str());
        int size =
            offsetof(struct sockaddr_un, sun_path) + strlen(serverConfig.sun_path);
        if (int ret = bind(fileTransferFd_, (struct sockaddr *)&serverConfig, size) < 0)
        {
            std::cout << ret << std::endl;
            throw "Error occurred while binding file Socket fd!";
        }

        if (listen(fileTransferFd_, 20) < 0)
        {
            throw "Error occurred while binding file Socket fd!";
        }
    }

    P2PClientBase::~P2PClientBase()
    {
        std::cout<<"~P2PClientBase()"<<std::endl;
        close(contactFd_);
        close(fileTransferFd_);
        unlink(FileTransferSocketPath_.c_str());
    }

    void P2PClientBase::handleNewFileTransFd()
    {
        struct sockaddr_un clientConfig;
        socklen_t clientLength = sizeof(clientConfig);
        int fd;
        if ((fd = accept(fileTransferFd_, (struct sockaddr *)&clientConfig,
                         &clientLength)) < 0)
        {
            throw "Error occurred while accepting new file Socket fd!";
        }

        int end = 0;
        int now = 0;
        int spaceCount = 0;
        while (true)
        {
            int ret = read(fd, fileBuff_, MAX_FILE_BUFFER);
            if (ret == 0)
            {
                throw "Error occurred while receving tranfers info!";
            }
            end += ret;
            for (int i = now; i < end; i++)
            {
                if (fileBuff_[i] == ' ')
                    spaceCount++;
            }
            if (spaceCount == 2)
                break;
            now = end;
        }

        char filePath[256];
        memset(filePath, 0, 256);
        int fileLength;
        sscanf(fileBuff_, "%s %d ", filePath, &fileLength);

        mapFd2FileTransFerInfo_[fd] =
            std::make_shared<FileTransFerInfo>(fd, filePath, fileLength);
    }

    void P2PClientBase::handleReadFileTransfer(
        std::shared_ptr<FileTransFerInfo> it)
    {
        int ret = read(it->fd, fileBuff_, MAX_FILE_BUFFER);
        Dict dict;
        dict["FileName"] = it->fileName;
        dict["FileLength"] = std::to_string(it->length);
        dict["AlreadySentLength"] = std::to_string(it->alreadySentLength);
        dict["IdentifyId"] = std::to_string(it->fd);

        newMessage_ += HptpContext::makeMessage(std::string(fileBuff_, ret), "",
                                                "", FILETRANSFER, dict);

        if (ret == 0)
        {
            removeFdFromSet(it->fd);
        }
    }

    void P2PClientBase::handleFileTransferMessage(std::shared_ptr<HptpContext> it)
    {
        if (!it->getValueByKey("Confirmed").empty()) // For sender
        {
            int localSockFd = stoi(it->getValueByKey("IdentifyId"));
            std::string bytesHaveReceived = it->getValueByKey("Confirmed") + " ";
            mapFd2FileTransFerInfo_[localSockFd]->alreadySentLength = stoi(it->getValueByKey("Confirmed"));
            write(localSockFd, bytesHaveReceived.c_str(), bytesHaveReceived.size());
            write(localSockFd, " ", 1);
        }
        else if (!it->getValueByKey("AlreadySentLength").empty()) // For receiver
        {
            int identifyId = stoi(it->getValueByKey("IdentifyId"));
            if (mapIdentify2FilePtr_.find(identifyId) == mapIdentify2FilePtr_.end())
            {
                FILE *filePtr = fopen(it->getValueByKey("FileName").c_str(), "a");
                if (filePtr == nullptr)
                {
                    throw "Creat file error!";
                }
                mapIdentify2FilePtr_[identifyId] = filePtr;
            }

            if (it->getValueByKey("Length") == "0")
            {
                fclose(mapIdentify2FilePtr_[identifyId]);
                mapIdentify2FilePtr_.erase(identifyId);
            }
            else
            {
                FILE *filePtr = mapIdentify2FilePtr_[identifyId];
                int ret = fwrite(it->getText().c_str(), 1, it->getText().length(), filePtr);
                int confirmed = ret + stoi(it->getValueByKey("AlreadySentLength"));
                Dict dict;
                dict["IdentifyId"] = it->getValueByKey("IdentifyId");
                dict["Confirmed"] = std::to_string(confirmed);
                newMessage_ += HptpContext::makeMessage("", "", "", FILETRANSFER, dict);
            }
        }
    }
} // namespace Raven
