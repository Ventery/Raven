#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/stat.h>

#include "../Base/Global.h"
#include "../Raven/Util.h"
using namespace std;

void usage();
void fileNotReadable();
void isNotFile();
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        usage();
    }

    if (access(argv[1], W_OK) == -1)
    {
        fileNotReadable();
    }

    struct stat s_buf;
    stat(argv[1], &s_buf);
    if (!S_ISREG(s_buf.st_mode))
    {
        isNotFile();
    }

    string fullPath = string(argv[1]);
    string fileName = fullPath.substr(fullPath.find_last_of("/") + 1);

    cout << "File name is : " << fileName << endl;
    return 0;
}

void usage()
{
    cout << "Usage: TestFileTransfer [FilePath]" << endl;
    exit(0);
}

void fileNotReadable()
{
    cout << "Path is not exist or readable." << endl;
    exit(0);
}

void isNotFile()
{
    cout << "Input is not a file,please check it." << endl;
    exit(0);
}