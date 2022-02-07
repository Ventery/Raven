#include <iostream>
#include "../P2PHost.h"

int main()
{
    while (Raven::P2PHost::isContinuous())
    {
        try
        {
            Raven::P2PHost host(RavenConfigIns.hostPortFrom_,
                                RavenConfigIns.serverIp_,
                                RavenConfigIns.serverPort_);
            host.init();
            host.run();
        }
        catch (const char *msg)
        {
            std::cout << msg << std::endl;
        }
    }

    return 0;
}