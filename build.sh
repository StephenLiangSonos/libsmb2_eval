#!/bin/bash

# a helper script to build both the library and test applications

LIB_DIR=/sonos/src/libsmb2
LIB_BUILD_DIR=lib_build

CWD=`pwd`

#export TOOLCHAIN=powerpc-fenwayV3-linux-gnu-
#export TOOLCHAIN=arm-ca7v3-linux-gnueabihf-
export CC=${TOOLCHAIN}gcc
export CXX=${TOOLCHAIN}g++
#export FLAGS="-mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -fstack-protector-strong -Wformat -Wformat-security -Werror=format-security"
#export FLAGS="-mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -fstack-protector-all -Wformat -Wformat-security -Werror=format-security"
export FLAGS="-g -fstack-protector-all -Wformat -Wformat-security -Werror=format-security"

# build libsmb2
#rm -rf ${LIB_BUILD_DIR}
mkdir -p ${LIB_BUILD_DIR}
cd ${LIB_BUILD_DIR}
cmake -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS} ${FLAGS}" -DCMAKE_INSTALL_PREFIX=. -DBUILD_SHARED_LIBS=0 ${LIB_DIR}
make install -j8

# build eval codes
cd $CWD
make clean
LIBSMB_BASE=${LIB_BUILD_DIR} make -j8
