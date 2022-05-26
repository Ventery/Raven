# Raven
[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)  [![Build Status](https://api.travis-ci.com/Ventery/Raven.svg?branch=master)](https://travis-ci.com/github/Ventery/Raven)
## [中文](https://github.com/Ventery/Raven/blob/master/README.md#%E4%B8%AD%E6%96%87)
## [English](https://github.com/Ventery/Raven/blob/master/README.md#introduce)

### 介绍
Raven类似于一个实现了NAT穿透的简化版SSH，由C++编写。

- 使用AES加密。
- 使用P2P模式，有服务器中转模式作为替代
- 服务器为linux，客户端和主机均支持linux和macos。
- 支持直接选取文件上传和下载。

### 一、安装准备工作
您需要：
-	一台有公网ip的linux服务器作为Server（也可以是内网ip，只要其余两台机器都能主动访问到它）；
-	两台linux或macos机器，用于控制（作为Client）和被控制（作为Host），做实验的话一台也可，即自己控制自己。
（如果你只对伪终端感兴趣或者只想做实验，甚至可以三合一，只用一台机器既当Server又当Host又当Client，注意此时只能用linux机器。）
-	上述机器上都要安装libgcrypt库(版本>=1.5.3)，我们使用这个来进行AES加密

libgcrypt安装：

macos：
```
brew update
brew install libgcrypt
```

centos：
```
wget http://mirror.centos.org/centos/7/os/x86_64/Packages/libgcrypt-devel-1.5.3-14.el7.x86_64.rpm
rpm -ivh libgcrypt-devel-1.5.3-14.el7.x86_64.rpm
```

ubuntu :
```
apt-get update
apt-get install libgcrypt-dev
```

找不到库的可以试着换下源。

其他系统的话自己找地方下，主要是把libgcrypt-dev安装好，然后gcrypt.h的位置可以找到基本就可以了。

### 二、安装：
-	下载Raven并解压进入目录或者直接pull，打开Raven.conf文件，修改server_ip为你的服务器ip，其他的你可以不用变。有安全需求的话，identify_key、aes_key_to_peer、aes_key_to_server三个字符串为16字节随机字符串可以自己生成。修改完毕后保存。log_path可以先不管。
-	对三台机器均作出以上操作，保证Raven.conf一样即可。然后在项目目录执行./build.sh开始编译，脚本会创建几个目录然后开始编译并在/usr/local/bin生成可执行文件的链接,可能需要一定权限。脚本创建的目录：

          1、在上级目录../建立../build目录作为build目录。

          2、建立～/conf/目录（在你的home目录下）作为配置文件目录并将Raven.conf复制过去，后续只能在这个文件夹修改conf，重启程序即可生效。

          3、建立～/RavenTrans目录，用于存放上传/下载的文件以及IPC的socket。
          
 ### 三、运行
 -	在server上执行TestRavenServer,在host上执行TestRavenHost,在client上执行TestRaClient。顺利的话client能够通过server连接到Host的shell上。
 -  下载文件:client连接上host的shell后，执行TestFileTransfer [文件名],选择一个socket回车（这里是当前host的本地socket）,然后文件会被下载到client机器~/RavenTrans目录下（利用当前的加密传输连接）。
 -  上传文件:client连接上host的shell后，在client机器上另外开一个shell，执行TestFileTransfer [文件名],选择一个socket回车（这里是当前client的本地socket）,然后文件会被上传到client机器~/RavenTrans目录下（利用当前的加密传输连接）。

//以前那版是黑历史，希望各位忘了吧。



### Introduce
Raven works like a simplified SSH with NAT traversal,written in C++.
- AES encryption.
- Use server as mid transprot.
- Server is linux only,host and client can be run in both linux and macos.
- Upload and download file in a visual way(compared with ssh).

### I、Preparation
You need：
-	A linux instance with public IPV4 IP.
-	Tow instances with macos or linux.
-	Install libgcrypt(version>=1.5.3) in all instances above.

libgcrypt installation：

for macos：
```
brew update
brew install libgcrypt
```

for centos：
```
wget http://mirror.centos.org/centos/7/os/x86_64/Packages/libgcrypt-devel-1.5.3-14.el7.x86_64.rpm
rpm -ivh libgcrypt-devel-1.5.3-14.el7.x86_64.rpm
```

for ubuntu :
```
apt-get update
apt-get install libgcrypt-dev
```

If you can not find libgcrypt please update or replace your sources.

After installation ,your program should find gcrypt.h

### II、Installation：
-	Download or pull Raven and enter the directory.Open Raven.conf and modify server_ip to your server ip，all the others can be stay the same.But I recommend you rebuild these 16 bytes string : identify_key、aes_key_to_peer、aes_key_to_server.Then save all.Some variables like log_path is not used yet so leave them aside。
-	You should do the above operations in all three instances and ensure that their Raven.conf is the same copy.Then excute ./build.sh in each directory to begin complile.The script will creat several directories and generate links in /usr/local/bin which point to executable files. Directories will be created:


          1、In parent directory creat ../build/ as build directory.

          2、Creat ～/conf/ as configure directory and copy Raven.conf to here.Then you can only modify this Raven.conf.Restart the program to make the modifying work.

          3、Creat ～/RavenTrans to store upload/download files and IPC sockets.
          
 ### III、Run
 -	In server excute TestRavenServer.In host excute TestRavenHost.In client excute TestRaClient.Client should connect to host's shell through server.
 -  Download file: After client connected to host, execute TestFileTransfer [filename] and then select one socket(host's local server socket) and press enter.Then the file will be downloaded to client's ~/RavenTrans/（use the very connection between client and host）.
 -  Upload file :After client connected to host，execute TestFileTransfer [filename] and then select one socket(client's local server socket) and press enter.Then the file will be upload to host's ~/RavenTrans/（use the very connection between client and host）.



//The former version was my bad work，please forget it.

