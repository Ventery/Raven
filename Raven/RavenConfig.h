//role : Config class.
//Author : Ventery

#ifndef RAVEN_RAVENCONFIG_H
#define RAVEN_RAVENCONFIG_H

#include <string>

#include "Util.h"
#include "../Base/Singleton.h"

namespace Raven
{
	class RavenConfig : public Global::Singleton<RavenConfig>
	{
	public:
		RavenConfig();
		~RavenConfig();

		std::string serverIp_;
		int serverPort_;
		std::string serverLogPath_;
		std::string aesKeyToServer_;
		int hostPortFrom_;
		int hostPortTo_;
		std::string hostLogPath_;
		int clientPort_;
		int keepAliveSec_;
		int connectTimeout_;
		std::string identifyKey_;
		std::string aesKeyToPeer_;
	};
} //namespace Raven

#endif //RAVEN_RAVENCONFIG_H