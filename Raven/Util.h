//role : Some common functions
//Author : Ventery

#ifndef RAVEN_BASE_UTIL_H
#define RAVEN_BASE_UTIL_H

#include <iostream>

#include "../Base/Global.h"
#define RavenConfigIns Raven::RavenConfig::GetInstance()
namespace Raven
{
	using namespace Global;
	typedef std::map<std::string, std::string> Dict;
//Definiton
	//enum for parse HPTP

	enum ProgressState
	{
		STATE_BEGIN = 1,
		STATE_GETTING_INFO,
		STATE_PROCESSING,
		STATE_END,
	};

	enum HPTPMessageType
	{
		PLAINTEXT = 100,		 
		PLAINTEXT_WINCTL = 101,	 
		CIPHERTEXT = 200,		 
		KEEPALIVE = 300,		 
		TRANSFER = 400,
	};

	enum SocketState
	{
		STATE_PARSE_PROTOCOL = 1, 
		STATE_PARSE_HEADERS,	  
		STATE_PARSE_TEXT,		  
	};

	enum ConnectionState
	{
		STATE_CONNECTED = 1, 
		STATE_DISCONNECTING, 
		STATE_DISCONNECTED,	 
		STATE_ERROR,		 
	};

	enum MessageState
	{
		PARSE_AGAIN = 1,
		PARSE_SUCCESS,
		PARSE_SUCCESS_KEEPALIVE,
		PARSE_SUCCESS_TRANSFER,
		PARSE_ERROR_PROTOCOL,
		PARSE_ERROR_HEADER,
		PARSE_ERROR_TEXT
	};

	enum ProtocolState
	{
		PARSE_PROTOCOL_AGAIN = 1,
		PARSE_PROTOCOL_ERROR,
		PARSE_PROTOCOL_SUCCESS_KEEPALIVE,
		PARSE_PROTOCOL_SUCCESS,
	};

	enum HeaderState
	{
		PARSE_HEADER_AGAIN = 1,
		PARSE_HEADER_ERROR,
		PARSE_HEADER_SUCCESS,
	};

	enum TextState
	{ 
		PARSE_TEXT_AGAIN = 1,
		PARSE_TEXT_SUCCESS,
		PARSE_TEXT_ERROR
	};

//Var
	//const var
	extern const std::string kMsgTooMuch;
	extern const std::string kKeepaliveMessage;
	extern const std::string kMsgPeerClosed;
	
	//var
	extern std::set<int> keepAliveFds;
	extern MutexLock m_keepAliveFds;

//Func
	PeerInfo getPeerInfo(const int &port, const bool &useCipher,const EndPointType &);
	bool noBlockConnect(const int &fd, const struct PeerInfo &peer, const int &timeOutSec);
	void addAlarm(const int &fd);
	void delAlarm(const int &fd);
	void handleAlarm(int sig);
} // namespace Raven

#endif // RAVEN_BASE_UTIL_H
