//role : HPTP context class.
//Author : Ventery
#ifndef RAVEN_HPTPCONTEXT_H
#define RAVEN_HPTPCONTEXT_H

#include "Util.h"
#include "../Base/noncopyable.h"

namespace Raven
{
	class HptpContext : public Global::Noncopyable
	{
		typedef std::map<std::string, std::string> Dict;

	public:
		HptpContext(int fd, std::string ip = "", int port = -1) : sockInfo_(fd, ip, port){};
		~HptpContext(){};

		//static
		static std::string makeMessage(const std::string &msg, const std::string &key, const std::string &iv, const HPTPMessageType &textType, const Dict &addtionHeaders = Dict());

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
		const std::string getAddress() { return sockInfo_.ip + " " + std::to_string(sockInfo_.port); }
		const int &getPeerSock() { return sockInfo_.peerSock; }
		void setPeerSock(const int &fd) { sockInfo_.peerSock = fd; }
		const std::string &getIdentifyKey() { return sockInfo_.identifyKey; }
		void setIdentifyKey(const std::string &key) { sockInfo_.identifyKey = key; }

	private:
		struct SockInfo
		{
			SockInfo() = delete;
			SockInfo(int fd, std::string ip, unsigned int port) : sock(fd),
																  peerSock(-4396),
																  ip(ip),
																  port(port),
																  textType(PLAINTEXT),
																  sockState(STATE_PARSE_PROTOCOL),
																  connState(STATE_CONNECTED),
																  lastEvent(DEFAULT_EPOLL_EVENT) {}
			int sock;
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
		CiphertextState parseCiphertext();

		struct SockInfo sockInfo_;
	};
} //namespace Raven

#endif //RAVEN_HPTPCONTEXT_H