#include <getopt.h>
#include <stdio.h>
#include "../P2PClient.h"
static void usage();
int main(int argc, char *argv[])
{
    try
    {
        Raven::P2PClient client(RavenConfigIns.clientPort_,
                                RavenConfigIns.serverIp_,
                                RavenConfigIns.serverPort_,
                                Global::EndPointType::TYPE_CLIENT);
        client.init();
        client.run();
    }
    catch (const char *msg)
    {
        std::cout << msg << std::endl;
    }

    return 0;
}