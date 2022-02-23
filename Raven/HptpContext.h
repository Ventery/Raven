//role : HPTP context class.
//Author : Ventery
#ifndef RAVEN_HPTPCONTEXT_H
#define RAVEN_HPTPCONTEXT_H

#include "Util.h"
#include "../Base/noncopyable.h"

namespace Raven
{
	static const Dict emptyDict;

	class HptpContext : public Global::Noncopyable
	{
	public:
		HptpContext(int fd, const std::string aesKey, const bool isHost = false, std::string ip = "", int port = -1) : sockInfo_(fd, aesKey, isHost, ip, port){};
		~HptpContext(){};

		//static
		static std::string makeMessage(const std::string &msg, const std::string &key, const std::string &iv, const HPTPMessageType &textType, const Dict &addtionHeaders = emptyDict);

		MessageState parseMessage();

		int readBlock();
		int readNoBlock(bool &zero);
		int writeBlock(const std::string &message);
		int writeNoBlock();
		void pushToWriteBuff(const std::string &message);

		//get&&set
		bool isReadBufferEmpty() { return sockInfo_.readBuffer.empty(); }
		bool isWriteBufferEmpty() { return sockInfo_.writeBuffer.empty(); }
		const std::string &getText() { return sockInfo_.payload; }
		const std::string &getValueByKey(const std::string &key) { return sockInfo_.headers[key]; }
		const HPTPMessageType &getCurrentTextType() { return sockInfo_.textType; }
		const ConnectionState &getConnectionState() { return sockInfo_.connState; }
		void setConnectionState(const ConnectionState &state) { sockInfo_.connState = state; }
		const __uint32_t &getLastEvent() { return sockInfo_.lastEvent; }
		void setLastEvent(const __uint32_t &event) { sockInfo_.lastEvent = event; }
		const int &getSock() { return sockInfo_.sock; }
		const std::string &getIp() { return sockInfo_.ip; }
		const unsigned int &getPort() { return sockInfo_.port; }

		const int &getPeerSock() { return sockInfo_.peerSock; }
		void setPeerSock(const int &fd) { sockInfo_.peerSock = fd; }
		const std::string &getIdentifyKey() { return sockInfo_.identifyKey; }
		void setIdentifyKey(const std::string &key) { sockInfo_.identifyKey = key; }
		const std::string &getAesKey() { return sockInfo_.aesKey; }
		const bool &isHost() { return sockInfo_.isHost; }
		void setIsHost(bool isHost) { sockInfo_.isHost = isHost; }

	private:
		struct SockInfo
		{
			SockInfo() = delete;
			SockInfo(int fd, const std::string &aesKey, const bool &isHost, std::string ip, unsigned int port) : sock(fd),
																												 aesKey(aesKey),
																												 isHost(isHost),
																												 peerSock(-4396),
																												 ip(ip),
																												 port(port),
																												 textType(PLAINTEXT),
																												 sockState(STATE_PARSE_PROTOCOL),
																												 connState(STATE_CONNECTED),
																												 lastEvent(DEFAULT_EPOLL_EVENT) {}
			int sock;
			std::string aesKey;
			bool isHost;
			int peerSock;
			std::string ip;
			unsigned int port;
			HPTPMessageType textType;
			std::string identifyKey;

			std::string readBuffer;
			std::string writeBuffer;
			std::string payload;

			SocketState sockState;
			ConnectionState connState;

			Dict headers;
			__uint32_t lastEvent;
		};

		ProtocolState parseProtocol();
		HeaderState parseHeader();
		TextState parseText();

		struct SockInfo sockInfo_;
	};
} //namespace Raven

#endif //RAVEN_HPTPCONTEXT_H