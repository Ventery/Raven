#include <iostream>
#include "../P2PClient.h"

int main()
{
    try
    {
        Raven::P2PClient client(RavenConfigIns.clientPort_,
                                RavenConfigIns.serverIp_,
                                RavenConfigIns.serverPort_);
        client.init();
        client.run();
    }
    catch (const char *msg)
    {
        std::cout << msg << std::endl;
    }

    return 0;
}