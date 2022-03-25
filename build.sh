#!/bin/sh
result=$(id | awk '{print $1}')
if [ $result != "uid=0(root)" ]; then
    echo " 请以root身份执行该脚本"
    exit 1
fi
set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}
BUILD_TYPE=${BUILD_TYPE:-Debug}

#For config file
CONF_DIR=$HOME"/conf/"
CONF_NAME="Raven.conf"
if [ ! -d "$CONF_DIR" ]; then
  mkdir $CONF_DIR
fi
if [ ! -f "$CONF_DIR$CONF_NAME" ]; then
  cp $CONF_NAME $CONF_DIR
fi

#For file transmission
FILE_DIR=$HOME"/RavenTrans/"    

if [ ! -d "$FILE_DIR" ]; then
  mkdir $FILE_DIR
fi

mkdir -p $BUILD_DIR/$BUILD_TYPE"_Raven" \
    && cd $BUILD_DIR/$BUILD_TYPE"_Raven" \
    && cmake \
            -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
            $SOURCE_DIR \
    && make $* \
    && CURRENT_DIR=`pwd` \
    && sudo ln -s -f $CURRENT_DIR/Host/Test/TestRavenHost /usr/local/bin/TestRavenHost \
    && sudo ln -s -f $CURRENT_DIR/Client/Test/TestRavenClient /usr/local/bin/TestRavenClient \
    && sudo ln -s -f $CURRENT_DIR/FileTransfer/TestFileTransfer /usr/local/bin/TestFileTransfer \
    && sudo ln -s -f $CURRENT_DIR/Server/Test/TestRavenServer /usr/local/bin/TestRavenServer \
