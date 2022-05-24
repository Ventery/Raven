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

    void P2PClientBase::createTransferSocket()
    {
        FileTransferSocketPath_ = kFileTransferPath + generateStr(8) + "_server.socket";
        std::cout << "FileTransferSocket :" << FileTransferSocketPath_ << std::endl;

        struct sockaddr_un serverConfig;

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
            int ret = read(fd, fileBuff_ + now, MAX_FILE_BUFFER);
            end += ret;
            fileBuff_[end] = 0;
            // std::cout << "Size : " << end << " " << fileBuff_ << std::endl;
            if (ret == 0)
            {
                throw "Error occurred while receving tranfers info!";
            }
            for (int i = now; i < end; i++)
            {
                if (fileBuff_[i] == ' ')
                    spaceCount++;
            }
            if (spaceCount == 2)
                break;
            now = end;
        }

        char empty;
        write(fd, &empty, 1); // sync

        char filePath[256];
        memset(filePath, 0, 256);
        int fileLength;
        sscanf(fileBuff_, "%s %d ", filePath, &fileLength);
        // std::cout << "Begin File Transfer " << std::endl
        //<< "File : " << filePath << "  Size: " << fileLength << std::endl;
        mapFd2FileTransFerInfo_[fd] =
            std::make_shared<FileTransFerInfo>(fd, filePath, fileLength);

        addFileFdToSet(fd);
    }

    void P2PClientBase::handleReadFileTransfer(
        std::shared_ptr<FileTransFerInfo> it)
    {
        int ret = read(it->fd, fileBuff_, MAX_BUFF);
        //std::cout << "file fd data in!  : " << it->fd << "  bytes:" << ret << std::endl;
        
        if (ret == 0)
        {
            removeFileFdFromSet(it->fd);
        }

        Dict dict;
        dict["FileName"] = it->fileName;
        dict["IdentifyId"] = std::to_string(it->fd);

        newMessage_ += HptpContext::makeMessage(std::string(fileBuff_, ret), "",
                                                "", FILETRANSFER, dict);
    }

    void P2PClientBase::handleFileTransferMessage(std::shared_ptr<HptpContext> it)
    {
        if (!it->getValueByKey("Confirmed").empty()) // For sender from receiver
        {
            int localSockFd = stoi(it->getValueByKey("IdentifyId"));
            long confirmed = stoi(it->getValueByKey("Confirmed"));
            auto info = mapFd2FileTransFerInfo_[localSockFd];
            info->alreadySentLength += confirmed;

            std::string bytesHaveReceived = std::to_string(info->alreadySentLength) + " "; // space for message end.
            //std::cout << "Confirmed :" << bytesHaveReceived << std::endl;
            write(localSockFd, bytesHaveReceived.c_str(), bytesHaveReceived.size());
            //std::cout << "Host write to FileTransfer!" << std::endl;
        }
        else  // For receiver from sender
        {
            //std::cout << "Client received!" << std::endl;

            int identifyId = stoi(it->getValueByKey("IdentifyId"));
            std::size_t textLength = it->getText().length();
            if (mapIdentify2FilePtr_.find(identifyId) == mapIdentify2FilePtr_.end())
            {
                FILE *filePtr = fopen((kFileTransferPath + it->getValueByKey("FileName")).c_str(), "w");
                if (filePtr == nullptr)
                {
                    throw "Creat file error!";
                }
                mapIdentify2FilePtr_[identifyId] = filePtr;
            }

            FILE *filePtr = mapIdentify2FilePtr_[identifyId];
            if (textLength == 0)    //Trans over
            {
                fflush(filePtr);
                fclose(filePtr);
                mapIdentify2FilePtr_.erase(identifyId);
                return ;
            }

            const char *buffPtr = it->getText().c_str();
            std::size_t bytesHadWroten = 0;
            while (bytesHadWroten < textLength)
            {
                bytesHadWroten += fwrite(buffPtr + bytesHadWroten, 1, textLength - bytesHadWroten, filePtr);
            }
            long confirmed = bytesHadWroten;
            Dict dict;
            dict["IdentifyId"] = it->getValueByKey("IdentifyId");
            dict["Confirmed"] = std::to_string(confirmed);
            newMessage_ += HptpContext::makeMessage("", "", "", FILETRANSFER, dict);
            //std::cout << "Client return to host! confirmd: "<< confirmed << std::endl;
        }
    }
} // namespace Raven
