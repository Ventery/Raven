#include <unistd.h>
#include <iostream>
#include <string>

#include "../Base/Global.h"
#include "../Raven/Util.h"
using namespace std;

void usage();
void fileNotReadable();
int main(int argc, char *argv[])
{
    if (argc <2){
        usage();
    }

    if (access(argv[1],W_OK) == -1){
        fileNotReadable();
    }
    string fullPath = string(argv[1]);
    string fileName = fullPath.substr(fullPath.find_last_of("/"));
    
    cout<<"File name is : "<<fileName<<endl;
    return 0;
}

void usage()
{
    cout<<"Usage: TestFileTransfer [FilePath]"<<endl;
    exit(0);
}

void fileNotReadable(){
    cout<<"File is not exist or readable!"<<endl;
    exit(0);
}