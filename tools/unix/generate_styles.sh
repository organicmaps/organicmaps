#!/bin/bash
set -e -u -x

MY_PATH=`pwd`

sh ./generate_symbols.sh
if [ $? -ne 0  ]; then
  echo "Error"
  exit 1 # error
fi

sh ./generate_drules.sh
if [ $? -ne 0  ]; then
  echo "Error"
  exit 1 # error
fi

echo "Done"
exit 0 # ok
