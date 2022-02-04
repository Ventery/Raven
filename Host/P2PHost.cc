#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <termios.h>

#include "P2PHost.h"

namespace Raven
{
    P2PHost::P2PHost(int localPort, std::string serverIp, int serverPort) : P2PClientBase(localPort, serverIp, serverPort), masterFd_(posix_openpt(O_RDWR))
    {
        runState_ = STATE_BEGIN;
        setSocketFD_CLOEXEC(masterFd_);
        grantpt(masterFd_);
        unlockpt(masterFd_);
        setNoBlocking(masterFd_);
        fdNum_ = 20;
    }

    void P2PHost::init()
    {
        slaveFd_ = open(ptsname(masterFd_), O_RDWR);
        bashPid_ = getBash(slaveFd_);
        sleep(1);
        formatTime("bash Pid = ");
        if (bashPid_ < 0)
        {
            throw "Bash start failed";
        }
        std::cout << bashPid_ << std::endl;

        runState_ = STATE_GETTING_INFO;
        P2PClientBase::init();
        context_ = std::make_shared<HptpContext>(contactFd_);
        ifDaemon();
        setSocketFD_CLOEXEC(publisherFd_);
        setSocketFD_CLOEXEC(subscriberFd_);
        setSocketFD_CLOEXEC(contactFd_);

        FD_ZERO(&oriReadSet);
        FD_ZERO(&oriWriteSet);
        FD_SET(contactFd_, &oriReadSet);
        FD_SET(masterFd_, &oriReadSet);
        FD_SET(subscriberFd_, &oriReadSet);

        addSig(SIGCHLD, std::bind(&P2PHost::signalHandler, this, SIGCHLD));
    }

    P2PHost::~P2PHost()
    {
        int status;
        wait(&status);
        if (WIFEXITED(status))
        {
            formatTime("Bash returns normally :");
            std::cout << WEXITSTATUS(status) << std::endl;
        }
        else
        {
            formatTime("Bash returns abnormally :");
            std::cout << WEXITSTATUS(status) << std::endl;
        }
        close(slaveFd_);
        close(masterFd_);
        close(contactFd_);
    }

    void P2PHost::run()
    {
        runState_ = STATE_PROCESSING;
        formatTime("Running...\n");
        isBashRunning = true;
        isRunning_ = true;
        addAlarm(contactFd_);

        while (isRunning_)
        {
            readSet = oriReadSet;
            writeSet = oriWriteSet;
            int rs = select(fdNum_, &readSet, &writeSet, 0, 0);
            if (rs < 0)
            {
                if (errno == EINTR)
                    continue;
                isRunning_ = false;
                formatTime("select() error! : ");
                std::cout << errno << std::endl;
            }
            else
            {
                if (FD_ISSET(contactFd_, &writeSet))
                {
                    handleWriteRemains();
                }
                if (FD_ISSET(masterFd_, &writeSet))
                {
                    handleBashWriteRemains();
                }
                if (FD_ISSET(subscriberFd_, &readSet))
                {
                    handleSignal();
                }
                if (!isRunning_)
                {
                    break;
                }

                newMessageToPeer_.clear();
                newMessageToBash_.clear();

                if (FD_ISSET(contactFd_, &readSet))
                {
                    handleRead();
                }
                if (FD_ISSET(masterFd_, &readSet))
                {
                    handleBashRead();
                }
                if (!newMessageToPeer_.empty())
                {
                    handleWrite();
                }
                if (!newMessageToBash_.empty())
                {
                    handleBashWrite();
                }
            }
        }

        delAlarm(contactFd_);
        handleStop();
    }

    void P2PHost::handleWriteRemains()
    {
        context_->writeNoBlock();
        if (context_->isWriteBufferEmpty())
        {
            FD_CLR(contactFd_, &oriWriteSet);
        }
    }

    void P2PHost::handleBashWriteRemains()
    {
        writen(masterFd_, MessageToBash_);
        if (MessageToBash_.empty())
        {
            FD_CLR(masterFd_, &oriWriteSet);
        }
    }

    void P2PHost::handleSignal()
    {
        int ret = read(subscriberFd_, buff_, MAX_BUFF);
        if (ret <= 0)
        {
            return;
        }
        else
        {
            for (int i = 0; i < ret; ++i)
            {
                formatTime("Received signal : ");
                switch (buff_[i])
                {
                case SIGTERM:
                case SIGINT:
                case SIGHUP:
                    std::cout << "Terminated signal" << std::endl;
                    isRunning_ = false;
                    continue;
                case SIGCHLD:
                    std::cout << "SIGCHLD" << std::endl;
                    isRunning_ = false;
                    isBashRunning = false;
                    continue;
                default:
                    std::cout << "Unknown signal : " << buff_[i] << std::endl;
                }
            }
        }
    }

    void P2PHost::handleRead()
    {
        bool zero = false;
        int readNum = context_->readNoBlock(zero);
        if (readNum < 0 || (zero && readNum == 0)) // something wrong
        {
            isRunning_ = false;
            formatTime("connection error! Prepare to reconnect!\n");
        }
        //select is Level Triggered.
        while (!context_->isReadBufferEmpty())
        {
            MessageState state = context_->parseMessage();

            if (state == PARSE_ERROR)
            {
                isRunning_ = false;
                break;
            }
            else if (state == PARSE_AGAIN)
            {
                break;
            }
            else if (state == PARSE_SUCCESS_KEEPALIVE)
            {
                continue;
            }
            else //(state==PARSE_SUCCESS
            {
                std::string tempMsg;
                if (context_->getCurrentTextType() == PLAINTEXT || context_->getCurrentTextType() == PLAINTEXT_WINCTL)
                {
                    tempMsg = context_->getText();
                }
                else
                {
                    tempMsg = decode(context_->getText(), RavenConfigIns.aesKeyToPeer_, context_->getValueByKey("iv"), stoi(context_->getValueByKey("length")));
                }
                if (context_->getCurrentTextType() == CIPHERTEXT_WINCTL || context_->getCurrentTextType() == PLAINTEXT_WINCTL)
                {
                    struct winsize size;
                    sscanf(tempMsg.c_str(), "%hu%hu", &(size.ws_row), &(size.ws_col));
                    if (ioctl(slaveFd_, TIOCSWINSZ, (char *)&size) < 0)
                    {
                        printf("TIOCGWINSZ error");
                    }
                }
                else
                {
                    newMessageToBash_ += tempMsg;
                }
            }
        }
    }

    void P2PHost::handleBashRead()
    {
        int ret = read(masterFd_, buff_, MAX_BUFF);
        newMessageToPeer_ += HptpContext::makeMessage(std::string(buff_, ret), RavenConfigIns.aesKeyToPeer_, generateStr(kBlockSize), CIPHERTEXT);
    }

    void P2PHost::handleWrite()
    {
        if (useTransfer)
        {
            newMessageToPeer_ = HptpContext::makeMessage(newMessageToPeer_, "", "", TRANSFER);
        }
        context_->pushToWriteBuff(newMessageToPeer_);
        context_->writeNoBlock();
        if (!context_->isWriteBufferEmpty())
        {
            FD_SET(contactFd_, &oriWriteSet);
        }
    }

    void P2PHost::handleBashWrite()
    {
        MessageToBash_ += newMessageToBash_;
        writen(masterFd_, MessageToBash_);
        if (!MessageToBash_.empty())
        {
            FD_SET(masterFd_, &oriWriteSet);
        }
    }

    void P2PHost::handleStop()
    {
        formatTime("Stopping......\n");
        if (isBashRunning)
        {
            kill(bashPid_,SIGHUP);//bash has it's own session,so we sent SIGHUP to it.
            std::string msg;
            setNoBlocking(subscriberFd_);
            read(subscriberFd_, buff_, MAX_BUFF); //handle remain signal
        }
        runState_ = STATE_END;
        formatTime("Stopped\n");
    }

    void P2PHost::signalHandler(int sig)
    {
        int saveErrno = errno;
        if (runState_ == STATE_GETTING_INFO)
        {
            close(contactFd_);
            exit(0);
        }
        if (sig != SIGHUP)
        {
            write(publisherFd_, (char *)&sig, 1);
        }
        if (sig != SIGCHLD && sig != SIGHUP)
        {
            isRunning_ = false;
        }
        errno = saveErrno;
    }
} //namespace Raven