//role : Client base class.
//Author : Ventery
#ifndef BASE_CLIENTBASE_H
#define BASE_CLIENTBASE_H
#include <string>

#include "EventHandlerBase.h"
#include "noncopyable.h"

namespace Global
{
	class ClientBase : public EventHandlerBase
	{
	public:
		ClientBase() = delete;
		ClientBase(int localPort, std::string serverIp, int serverPort);
		~ClientBase();

		virtual void init();
		virtual void run() = 0;

	protected:
		virtual void signalHandler(int sig){};
		virtual void handleSignal() = 0;
		virtual void handleRead() = 0;
		virtual void handleWrite() = 0;
		virtual void handleWriteRemains() = 0;

		int localPort_;
		std::string serverIp_;
		int serverPort_;
		bool isRunning_;
		int pipeFds_[2];
		int publisherFd_;
		int subscriberFd_;
		int contactFd_;
	};
} // namespace Global

#endif // BASE_CLIENTBASE_H