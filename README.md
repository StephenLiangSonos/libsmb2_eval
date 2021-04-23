This project contains evaluation code for libsmb2

To compile the test code, build and install libsmb2 first, then set the LIBSMB_BASE directory. The library exists on Github ```https://github.com/sahlberg/libsmb2.git```.

```
Example steps to compile for fenway with the powerpc-fenwayV3-linux-gnu toolchain, given the directories:
 - libsmb2 at /sonos/src/libsmb2
 - evaluation code at /sonos/src/libsmb2_eval
 - and temporary directory at /sonos/src/tmp

export TOOLCHAIN=powerpc-fenwayV3-linux-gnu-
export CC=${TOOLCHAIN}gcc
export CXX=${TOOLCHAIN}g++

cd /sonos/src/tmp
cmake -DCMAKE_INSTALL_PREFIX=. -DBUILD_SHARED_LIBS=0 /sonos/src/libsmb2
make install -j8

cd /sonos/src/libsmb2_eval
LIBSMB_BASE=/sonos/src/tmp make -j8
```

To run the test programs on a target platform, copy over to /jffs/ and run as follows:
```
# for syscall tests
program //IP/DIR username password version
e.g. ./dir_crawl_syscall //192.168.0.200/Music stephen password 1.0
e.g. ./dir_crawl_syscall //192.168.0.200/Music stephen password 2.0
e.g. ./dir_crawl_syscall //192.168.0.200/Music stephen password 3.0

# for libsmb2 tests
program smb://IP/DIR username password version
e.g. ./dir_crawl_libsmb2 smb://192.168.0.200/Music stephen password 2
e.g. ./dir_crawl_libsmb2 smb://192.168.0.200/Music stephen password 3
```

tmp directory needs to be cleaned up when switching toolchains.
