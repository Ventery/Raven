#include "P2PServer.h"

namespace Raven
{
    P2PServer::P2PServer(int listenPort) : ServerBase(listenPort)
    {
    }

    P2PServer::~P2PServer()
    {
    }

    void P2PServer::init()
    {
        ServerBase::init();
    }

    void P2PServer::run()
    {
        isRunning_ = true;
        formatTime("Server Running!\n");

        while (isRunning_)
        {
            int num = epoll_wait(epollFd_, events_, MAX_EVENT_NUMBER, -1);
            if ((num < 0) && (errno != EINTR))
            {
                formatTime("epoll failure!\n");
                isRunning_ = false;
                break;
            }

            for (int i = 0; i < num; i++)
            {
                if (!isRunning_)
                {
                    break;
                }
                int sockfd = events_[i].data.fd;
                if (sockfd == listenFd) //监听fd
                {
                    handleNewConnection();
                }
                else if ((events_[i].events & EPOLLIN) && (sockfd == subscriberFd_))
                {
                    handleSignal();
                }
                else if (events_[i].events & EPOLLHUP)
                {
                    socksToClose_.insert(sockfd);
                }
                else if (events_[i].events & EPOLLOUT)
                {
                    handleWrite(sockfd);
                }
                else if (events_[i].events & EPOLLIN)
                {
                    handleRead(sockfd);
                }
                else
                {
                    formatTime("");
                    std::cout << "sockfd : " << sockfd << " event: " << events_[i].events << " not handled" << std::endl;
                }
            }
            handleClose();
        }
    }

    void P2PServer::handleSignal()
    {
        char signals[1024];
        int ret = read(subscriberFd_, signals, sizeof(signals));
        if (ret <= 0)
        {
            return;
        }
        else
        {
            for (int i = 0; i < ret; ++i)
            {
                switch (signals[i])
                {
                case SIGHUP:
                case SIGTERM:
                case SIGINT:
                {
                    formatTime("Stopping server......\n");
                    isRunning_ = false;
                }
                }
            }
        }
    }

    void P2PServer::handleNewConnection()
    {
        while (true) //边缘触发模式需要不停accept
        {
            int newFd = accept(listenFd, (struct sockaddr *)&clientAddress_, &clientAddrlength_);
            if (newFd < 0)
            {
                if (errno == EAGAIN)
                    break;
                else
                {
                    perror("Listen fd error");
                    isRunning_ = false;
                    break;
                }
            }
            if (newFd > MAX_CONNECT_NUMBER)
            {
                std::string msg = kMsgTooMuch;
                writen(newFd, msg);
                close(newFd);
                continue;
            }
            formatTime("Come new connection! fd : ");
            std::cout << newFd << std::endl;
            mapSock2Address_[newFd] = std::make_shared<HptpContext>(newFd,
                                                                    std::string(inet_ntoa(clientAddress_.sin_addr)),
                                                                    ntohs(clientAddress_.sin_port));
            addFd(epollFd_, newFd);
        }
    }

    void P2PServer::handleRead(int fd)
    {
        assert(mapSock2Address_.find(fd) != mapSock2Address_.end());
        auto context = mapSock2Address_[fd];

        bool zero = false;
        int readNum = context->readNoBlock(zero);
        if (readNum < 0)
        {
            context->setConnectionState(STATE_ERROR);
            socksToClose_.insert(fd);
            return;
        }
        if (zero)
        {
            if (readNum == 0)
            {
                socksToClose_.insert(fd);
            }
            else
            {
                context->setConnectionState(STATE_DISCONNECTING);
            }
        }
        while (!context->isReadBufferEmpty())
        {
            MessageState state = context->parseMessage();
            if (state == PARSE_ERROR)
            {
                context->setConnectionState(STATE_ERROR);
                socksToClose_.insert(fd);
                return;
            }
            else if (state == PARSE_AGAIN)
            {
                break;
            }
            else if (state == PARSE_SUCCESS_KEEPALIVE)
            {
                continue;
            }
            else if (state == PARSE_SUCCESS_TRANSFER)
            {
                if (context->getPeerSock() > 0)
                {
                    auto peerContext = mapSock2Address_[context->getPeerSock()];
                    peerContext->pushToWriteBuff(context->getText());
                    handleWrite(context->getPeerSock());
                }
            }
            else //(state==PARSE_SUCCESS ,make peer
            {
                std::string identifyKey;
                auto &textType = context->getCurrentTextType();
                if (textType == PLAINTEXT)
                {
                    identifyKey = context->getText();
                }
                else if (textType == CIPHERTEXT)
                {
                    identifyKey = decode(context->getText(), RavenConfigIns.aesKeyToServer_, context->getValueByKey("iv"), stoi(context->getValueByKey("length")));
                }
                else
                {
                    socksToClose_.insert(fd);
                    break;
                }
                context->setIdentifyKey(identifyKey);
                auto it = mapIdkey2Address_.find(identifyKey); //Check whether the peer is online or not.
                if (it == mapIdkey2Address_.end())
                {
                    mapIdkey2Address_[identifyKey] = context;
                }
                else
                {
                    auto peerContext = it->second;
                    context->setPeerSock(peerContext->getSock());
                    peerContext->setPeerSock(context->getSock());
                    std::string message;
                    context->pushToWriteBuff(HptpContext::makeMessage(peerContext->getAddress(), RavenConfigIns.aesKeyToServer_, generateStr(kBlockSize), CIPHERTEXT));
                    handleWrite(context->getSock());
                    peerContext->pushToWriteBuff(HptpContext::makeMessage(context->getAddress(), RavenConfigIns.aesKeyToServer_, generateStr(kBlockSize), CIPHERTEXT));
                    handleWrite(peerContext->getSock());

                    mapIdkey2Address_.erase(identifyKey);
                }
            }
        }
    }

    void P2PServer::handleWrite(int fd)
    {
        assert(mapSock2Address_.find(fd) != mapSock2Address_.end());
        auto context = mapSock2Address_[fd];
        context->writeNoBlock();

        __uint32_t eventToset;
        if (!context->isWriteBufferEmpty())
        {
            eventToset = DEFAULT_EPOLL_EVENT | EPOLLOUT;
        }
        else
        {
            eventToset = DEFAULT_EPOLL_EVENT;
            if (context->getConnectionState() == STATE_DISCONNECTING)
            {
                socksToClose_.insert(fd);
            }
        }
        if (eventToset != context->getLastEvent()) //防止频繁epoll_ctl更改监听事件
        {
            epoll_event event;
            event.data.fd = fd;
            event.events = eventToset;
            epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event);
            context->setLastEvent(eventToset);
        }
    }

    void P2PServer::handleClose()
    {
        std::set<int> allSocksToClose;
        for (auto it : socksToClose_)
        {
            if (mapSock2Address_.find(it) != mapSock2Address_.end())
            {
                auto context = mapSock2Address_[it];
                int peerSock = context->getPeerSock();
                if (peerSock > 0 && mapSock2Address_.find(peerSock) != mapSock2Address_.end())
                {
                    auto peerContext = mapSock2Address_[peerSock];
                    resetSock(peerContext);
                    allSocksToClose.insert(peerContext->getSock());
                }
                resetSock(context);
                allSocksToClose.insert(context->getSock());
                mapIdkey2Address_.erase(context->getIdentifyKey());
            }
        }

        for (auto it : allSocksToClose)
        {
            mapSock2Address_.erase(it);
        }

        socksToClose_.clear();
    }

    void P2PServer::resetSock(std::shared_ptr<HptpContext> &context)
    {
        context->writeBlock(kMsgPeerClosed);
        removeFd(epollFd_, context->getSock());
    }
} // namespace Raven