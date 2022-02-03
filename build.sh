#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}
BUILD_TYPE=${BUILD_TYPE:-Debug}

CONF_DIR=$HOME"/conf/"
CONF_NAME="Raven.conf"

mkdir -p $CONF_DIR && cp $CONF_NAME $CONF_DIR


mkdir -p $BUILD_DIR/$BUILD_TYPE"_Raven" \
    && cd $BUILD_DIR/$BUILD_TYPE"_Raven" \
    && cmake \
            -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
            $SOURCE_DIR \
    && make $* \
    && CURRENT_DIR=`pwd` \
    && sudo ln -s -f $CURRENT_DIR/Server/Test/TestRavenServer /usr/local/bin/TestRavenServer \
    && sudo ln -s -f $CURRENT_DIR/Host/Test/TestRavenHost /usr/local/bin/TestRavenHost \
    && sudo ln -s -f $CURRENT_DIR/Client/Test/TestRavenClient /usr/local/bin/TestRavenClient \
