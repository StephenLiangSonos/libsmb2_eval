#!/bin/bash

LIB_DIR=/sonos/src/libsmb2
LIB_BUILD_DIR=lib_build

CWD=`pwd`

# build libsmb2
mkdir -p ${LIB_BUILD_DIR}
cd ${LIB_BUILD_DIR}
cmake -DCMAKE_INSTALL_PREFIX=. -DBUILD_SHARED_LIBS=0 ${LIB_DIR}
make install -j8

# build eval codes
cd $CWD
LIBSMB_BASE=${LIB_BUILD_DIR} make -j8
