#!/bin/bash

MY_PATH=$(dirname "$0")                 # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

source "$MY_PATH/tests_list.sh"

echo "Cleaning omim/andoid..."
#ndk-build --directory=$MY_PATH/.. clean

RemoveObjLibs() {
  rm -r $1/obj
  rm -r $1/libs
}

for test in "${TESTS_LIST[@]}"; do
  echo "Cleaning test $test"
  ndk-build --directory=$test clean
  RemoveObjLibs $test
done

ndk-build --directory=$MY_PATH clean
RemoveObjLibs $MY_PATH
rm -r $MY_PATH/build
