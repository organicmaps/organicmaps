#!/bin/bash

MY_PATH=$(dirname "$0")                 # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

source "$MY_PATH/tests_list.sh"


NDK_PATH=""
for line in $(< ./local.properties)
do
  case $line in
    ndk*)  NDK_PATH=`echo $line| cut -d'=' -f 2` ;;
    *) ;;
   esac
done



declare -r NDK_ABI=armeabi-v7a

declare -r PROP_DEBUG_NDK_FLAGS="-j8 V=1 NDK_DEBUG=1 DEBUG=1 APP_ABI=$NDK_ABI"
declare -r PROP_RELEASE_NDK_FLAGS="-j8 V=1 NDK_DEBUG=0 PRODUCTION=1 APP_ABI=$NDK_ABI"

Usage() {
  echo "Usage: $0 <debug|release|production>" 1>&2
  exit 1
}

NdkBuild() {
  if [ "$1" = "debug" ]; then
    NDK_PARAMS="--directory=$2 ${PROP_DEBUG_NDK_FLAGS}"
  else
    NDK_PARAMS="--directory=$2 ${PROP_RELEASE_NDK_FLAGS}"
  fi
  echo "ndk-build $NDK_PARAMS"
  $NDK_PATH/ndk-build $NDK_PARAMS
}

if [ $# != 1 ]; then
  Usage
fi

if [ "$1" != "debug" && "$1" != "release" && "$1" != "production" ]; then
  Usage
fi

echo "Building omim/andoid..."
if [ "$1" = "debug" ]; then
  bash $MY_PATH/../../tools/autobuild/android.sh debug $NDK_ABI
else
  bash $MY_PATH/../../tools/autobuild/android.sh production $NDK_ABI
fi

for test in "${TESTS_LIST[@]}"; do
  echo "Building test $test"
  NdkBuild $1 ${test}
done

echo "Copying so-files with tests to the libs directory: "
TMP_LIB_DIR=$MY_PATH/libs/tmp/$NDK_ABI/
echo $TMP_LIB_DIR
mkdir -p $TMP_LIB_DIR
for test in "${TESTS_LIST[@]}"; do
  echo "Copying so-file for test: $test"
  cp -r $test/libs/*/*.so $TMP_LIB_DIR
done

echo "Building main so file for the native activity... "
NdkBuild $1 .
