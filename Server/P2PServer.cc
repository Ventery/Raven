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
                                                                    RavenConfigIns.aesKeyToPeer_,
                                                                    false,
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
            //std::cout << state << std::endl;
            if (state >= PARSE_ERROR_PROTOCOL)
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
                int peerSock = context->getPeerSock();
                if (peerSock > 0)
                {
                    auto peerContext = mapSock2Address_[peerSock];
                    peerContext->pushToWriteBuff(context->getText());
                    handleWrite(peerSock);
                }
            }
            else //(state==PARSE_SUCCESS ,make peer
            {
                std::string identifyKey = context->getValueByKey("IdentifyKey");
                context->setIdentifyKey(identifyKey);
                bool isHost = stoi(context->getValueByKey("EndPointType"));
                context->setIsHost(isHost);
                //std::cout << "IdentifyKey : " << identifyKey << " isHost: " << isHost << std::endl;

                auto it = mapIdkey2Address_[1 - isHost].find(identifyKey); //Check whether the peer is online or not.
                if (it == mapIdkey2Address_[1 - isHost].end())
                {
                    if (mapIdkey2Address_[isHost].find(identifyKey) == mapIdkey2Address_[isHost].end())
                    {
                        mapIdkey2Address_[isHost][identifyKey] = context;
                    }
                    else
                    {
                        context->setConnectionState(STATE_ERROR);
                        socksToClose_.insert(fd);
                        return;
                    }
                }
                else
                {
                    auto peerContext = it->second;
                    context->setPeerSock(peerContext->getSock());
                    peerContext->setPeerSock(context->getSock());
                    std::string message;
                    Dict dict;
                    dict["PeerIp"] = peerContext->getIp();
                    dict["PeerPort"] = std::to_string(peerContext->getPort());
                    context->pushToWriteBuff(HptpContext::makeMessage(HptpContext::makeMessage("", "", "", PLAINTEXT, dict), context->getAesKey(), generateStr(kBlockSize), CIPHERTEXT));
                    handleWrite(context->getSock());

                    dict["PeerIp"] = context->getIp();
                    dict["PeerPort"] = std::to_string(context->getPort());
                    peerContext->pushToWriteBuff(HptpContext::makeMessage(HptpContext::makeMessage("", "", "", PLAINTEXT, dict), peerContext->getAesKey(), generateStr(kBlockSize), CIPHERTEXT));
                    handleWrite(peerContext->getSock());

                    mapIdkey2Address_[1 - isHost].erase(identifyKey);
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
                mapIdkey2Address_[context->isHost()].erase(context->getIdentifyKey());
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