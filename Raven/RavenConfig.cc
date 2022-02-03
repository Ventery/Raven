#include "RavenConfig.h"

namespace Raven
{
	RavenConfig::RavenConfig() : serverIp_(ConfigIns.Read("server_ip", std::string(""))),
								 serverPort_(ConfigIns.Read("server_port", -1)),
								 serverLogPath_(ConfigIns.Read("server_log_path", std::string("./Raven.log"))),
								 aesKeyToServer_(ConfigIns.Read("aes_key_to_server", std::string(""))),
								 hostPortFrom_(ConfigIns.Read("host_port_from", -1)),
								 hostPortTo_(ConfigIns.Read("host_port_to", -1)),
								 hostLogPath_(ConfigIns.Read("host_log_path", std::string("./Raven.log"))),
								 clientPort_(ConfigIns.Read("client_port", -1)),
								 keepAliveSec_(ConfigIns.Read("keep_alive_sec", 5)),
								 connectTimeout_(ConfigIns.Read("connect_timeout", 5)),
								 identifyKey_(ConfigIns.Read("identify_key", std::string(""))),
								 aesKeyToPeer_(ConfigIns.Read("aes_key_to_peer", std::string("")))
	{
		std::cout<<"+----Config  Analysis----+"<<std::endl;

		if (serverIp_ == "")
			throw("server_ip invalid!\n");
		std::cout<<"server_ip"<<" : "<<serverIp_<<std::endl;

		if (serverPort_ == -1)
			throw("server_port invalid!\n");
		std::cout<<"server_port"<<" : "<<serverPort_<<std::endl;

		if (aesKeyToServer_ == "")
			throw("aes_key_to_server invalid!\n");
		std::cout<<"aes_key_to_server"<<" : "<<aesKeyToServer_<<std::endl;

		if (hostPortFrom_ == -1)
			throw("host_port_from invalid!\n");
		std::cout<<"host_port_from"<<" : "<<hostPortFrom_<<std::endl;

		if (hostPortTo_ == -1)
			throw("host_port_to invalid!\n");
		std::cout<<"host_port_to"<<" : "<<hostPortTo_<<std::endl;

		if (clientPort_ == -1)
			throw("client_port invalid!\n");
		std::cout<<"client_port"<<" : "<<clientPort_<<std::endl;

		if (identifyKey_ == "")
			throw("identifyKey invalid!\n");
		std::cout<<"identifyKey"<<" : "<<identifyKey_<<std::endl;
		
		if (aesKeyToPeer_ == "")
			throw("aes_key_to_peer invalid!\n");
		std::cout<<"aes_key_to_peer"<<" : "<<aesKeyToPeer_<<std::endl;

		std::cout<<"+---Config check Done!---+"<<std::endl;
	}

} //namespace Raven