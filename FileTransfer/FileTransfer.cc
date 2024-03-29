#include <dirent.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

#include "../Base/Global.h"
#include "../Base/ScreenRefresher.h"
#include "../Raven/Util.h"
using namespace std;

void usage();
void fileNotReadable();
void isNotFile();
int connectToSocket(string, string &);
void beginTrans(string, string, int, struct stat &);

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

    struct stat statBuff;
    stat(argv[1], &statBuff);
    if (!S_ISREG(statBuff.st_mode))
    {
        isNotFile();
    }

    string fullPath = string(argv[1]);
    string fileName = fullPath.substr(fullPath.find_last_of("/") + 1);
    cout << "File name is : " << fileName << endl;

    vector<string> socketList;

    DIR *dir;
    struct dirent *ent;
    string TransferPath = Global::kFileTransferPath;
    if ((dir = opendir(TransferPath.c_str())) != NULL)
    {

        while ((ent = readdir(dir)) != NULL)
        {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
                continue;
            string fileName = string(ent->d_name);
            if (fileName.find("_server.socket") != string::npos)
            {
                socketList.push_back(ent->d_name);
            }
        }
        closedir(dir);
        cout << "--------------------------------" << endl;
        cout << "Raven file transfer path: " << TransferPath << endl;

        if (socketList.empty())
        {
            cout << "Please run RavenClient or RavenHost first!" << endl;
        }
        cout << "Local sockets:" << endl;
        for (size_t i = 0; i < socketList.size(); i++)
        {
            cout << i + 1 << " : " << socketList[i] << endl;
        }
        cout << "--------------------------------" << endl;
        cout << "Please select a socket,input index:" << endl;
        string socketIndex;
        while (true)
        {
            cin >> socketIndex;
            int index = stoi(socketIndex);
            if (index > 0 && index <= (int)(socketList.size()))
            {
                break;
            }
            else
            {
                cout << "Please input valid index:" << endl;
            }
        }

        string socket = socketList[stoi(socketIndex) - 1];
        string clientSocket;
        int clientFd = connectToSocket(socket, clientSocket);
        beginTrans(fullPath, fileName, clientFd, statBuff);
        close(clientFd);
        unlink(clientSocket.c_str());
    }
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

int connectToSocket(string serverSocket, string &clientSocket)
{
    clientSocket = Global::kFileTransferPath + serverSocket.substr(0, 8) + "_client.socket";
    struct sockaddr_un clientConfig, servefrConfig;

    int clientSockFd;
    if ((clientSockFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        perror("client socket error");
        exit(1);
    }

    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.sun_family = AF_UNIX;
    strcpy(clientConfig.sun_path, clientSocket.c_str());
    int len = offsetof(struct sockaddr_un, sun_path) + strlen(clientConfig.sun_path);
    unlink(clientConfig.sun_path);
    if (::bind(clientSockFd, (struct sockaddr *)&clientConfig, len) < 0)
    {
        perror("bind error");
        exit(1);
    }

    memset(&servefrConfig, 0, sizeof(servefrConfig));
    servefrConfig.sun_family = AF_UNIX;
    strcpy(servefrConfig.sun_path, (Global::kFileTransferPath + serverSocket).c_str());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(servefrConfig.sun_path);
    if (connect(clientSockFd, (struct sockaddr *)&servefrConfig, len) < 0)
    {
        std::cout << "Server socket : " << servefrConfig.sun_path << std::endl;
        perror("connect error");
        exit(1);
    }

    return clientSockFd;
}
int min(int a, int b) { return a < b ? a : b; }

void beginTrans(string fullPath, string fileName, int clientFd, struct stat &statBuff)
{
    int fileSize = statBuff.st_size;
    cout << "File size:" << fileSize << endl;
    write(clientFd, fileName.c_str(), fileName.length());
    write(clientFd, " ", 1);

    string tempStream = to_string(fileSize);
    write(clientFd, tempStream.c_str(), tempStream.length());
    write(clientFd, " ", 1);

    char empty;
    read(clientFd, &empty, 1); // sync

    // cout << "Begin trans:" << endl;

    FILE *filePtr = fopen(fullPath.c_str(), "r");
    int fileBlock = min(MAX_BUFF, statBuff.st_size / 10);
    char buff[fileBlock];
    char readBuff[NORMAL_BUFF];

    ScreenRefresher outputer("bytes", fileSize);
    long confirmedBytes = 0;

    while (true)
    {
        if (feof(filePtr))
        {
            cout << endl;
            cout << "Transmission complete!" << endl;
            close(clientFd);
            fclose(filePtr);

            break;
        }
        int ret = fread(buff, 1, fileBlock, filePtr);
        //cout << "Fread bytes:" << ret << endl;

        if (ret > 0)
        {
            long writeNum = 0;
            while (ret - writeNum)
            {
                writeNum += write(clientFd, buff + writeNum, ret - writeNum);
            }
             //cout << "Write bytes:" << writeNum << endl;
             long lastConfirmBytes = confirmedBytes;
             while (lastConfirmBytes + writeNum > confirmedBytes)
             {
                long readNum = 0;
                while (true)
                {
                    readNum += read(clientFd, readBuff + readNum, 1);
                    if (readBuff[readNum - 1] == ' ')
                    {
                        break;
                    }
                }
                sscanf(readBuff, "%ld ", &confirmedBytes);
                outputer.rePrint(confirmedBytes);
             }
        }
    }
}