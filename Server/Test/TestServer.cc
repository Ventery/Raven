#include <iostream>
#include"../P2PServer.h"

int main()
{
    try
    {
        Raven::P2PServer server(RavenConfigIns.serverPort_);
        server.init();
        server.run();
    }
    catch (const char *msg)
    {
        std::cout << msg << std::endl;
    }

    return 0;
}