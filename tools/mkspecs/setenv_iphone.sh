#!/bin/sh

BASE_SDK_VERSION=4.3
DEV_ROOT="/Developer/Platforms/iPhoneOS.platform/Developer"
SDK_ROOT="$DEV_ROOT/SDKs/iPhoneOS${BASE_SDK_VERSION}.sdk"
# minimum supported iphone os version at runtime
IOS_VERSION=4.0

export LANG=en_US.US-ASCII
export PATH=$DEV_ROOT/usr/bin:/Developer/usr/bin:/usr/bin:/bin:/usr/sbin:/sbin

export CC="gcc-4.2"
export CXX="g++-4.2"

C_AND_CXX_FLAGS="-arch armv6 -arch armv7 -fmessage-length=0 -pipe -fpascal-strings -Wall \
  -fvisibility=hidden -mno-thumb -miphoneos-version-min=$IOS_VERSION -gdwarf-2 \
  --sysroot=$SDK_ROOT -isystem $SDK_ROOT/usr/include -iwithsysroot $SDK_ROOT"

export CFLAGS="$C_AND_CXX_FLAGS -std=c99"
export CXXFLAGS="$C_AND_CXX_FLAGS -fvisibility-inlines-hidden"

# linker settings
export IPHONEOS_DEPLOYMENT_TARGET=$IOS_VERSION
# just to mention - this flag is passed to libtool, not to gcc
LIBTOOL_FLAGS="-syslibroot $SDK_ROOT"
LDFLAGS="-L$SDK_ROOT/usr/lib"
