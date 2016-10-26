#!/bin/sh

BASE_SDK_VERSION=4.3
DEV_ROOT="/Developer/Platforms/iPhoneSimulator.platform/Developer"
SDK_ROOT="$DEV_ROOT/SDKs/iPhoneSimulator${BASE_SDK_VERSION}.sdk"
# minimum supported iphone os version at runtime
IOS_VERSION=30100

export LANG=en_US.US-ASCII
export PATH=$DEV_ROOT/usr/bin:/Developer/usr/bin:/usr/bin:/bin:/usr/sbin:/sbin

export CC="gcc-4.2"
export CXX="g++-4.2"

C_AND_CXX_FLAGS="-arch i386 x86_64 -fmessage-length=0 -pipe -fpascal-strings -fasm-blocks -Wall \
  -fvisibility=hidden -mmacosx-version-min=10.6 -gdwarf-2 \
  -D__IPHONE_OS_VERSION_MIN_REQUIRED=$IOS_VERSION \
  -isysroot $SDK_ROOT -isystem $SDK_ROOT/usr/include"

export CFLAGS="$C_AND_CXX_FLAGS -std=c99"
export CXXFLAGS="$C_AND_CXX_FLAGS -fvisibility-inlines-hidden"

# linker settings
export MACOSX_DEPLOYMENT_TARGET=10.6
# just to mention - this flag is passed to libtool, not to gcc
LIBTOOL_FLAGS="-syslibroot $SDK_ROOT"
LDFLAGS="-L$SDK_ROOT/usr/lib"