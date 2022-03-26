// role : Client base class.
// Author : Ventery
#ifndef BASE_CLIENTBASE_H
#define BASE_CLIENTBASE_H
#include <string>

#include "EventHandlerBase.h"
#include "Global.h"
#include "noncopyable.h"

namespace Global
{
	class ClientBase : public EventHandlerBase
	{
	public:
		ClientBase() = delete;
		ClientBase(int localPort, std::string serverIp, int serverPort);
		~ClientBase();

		virtual void run() = 0;
		virtual void signalHandler(int sig) = 0;

	protected:
		virtual void handleSignal() = 0;
		virtual void handleRead() = 0;
		virtual void handleWrite() = 0;
		virtual void handleWriteRemains() = 0;

		template <class T>
		void init(T &t);

		int localPort_;
		std::string serverIp_;
		int serverPort_;
		bool isRunning_;
		int pipeFds_[2];
		int publisherFd_;
		int subscriberFd_;
		int contactFd_;
	};

	template <class T>
	void ClientBase::init(T &t)
	{
		addSig(SIGTERM, std::bind(&T::signalHandler, &t, SIGTERM));
		addSig(SIGINT, std::bind(&T::signalHandler, &t, SIGINT));
		addSig(SIGHUP, std::bind(&T::signalHandler, &t, SIGHUP));

		Global::printCurrentSystem();
		srand(kTimeSeed);
		Global::setNoBlocking(publisherFd_);
	}
} // namespace Global

#endif // BASE_CLIENTBASE_H