#include <iostream>
#include "../Base/Global.h"
#include "../Raven/Util.h"
using namespace std;

void usage();
int main(int argc, char *argv[])
{
    if (argc <2){
        usage();
    }
    return 0;
}

void usage()
{
    cout<<"Usage: TestFileTransfer [FilePath]"<<endl;
    exit(0);
}