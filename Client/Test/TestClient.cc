#include <getopt.h>
#include <stdio.h>
#include "../P2PClient.h"
int main(int argc, char *argv[])
{
    try
    {
        Raven::P2PClient client(RavenConfigIns.clientPort_,
                                RavenConfigIns.serverIp_,
                                RavenConfigIns.serverPort_,
                                Global::EndPointType::TYPE_CLIENT);
        client.init<Raven::P2PClient>(client);
        client.run();
    }
    catch (const char *msg)
    {
        std::cout <<"Client : "<< msg << std::endl;
    }

    return 0;
}