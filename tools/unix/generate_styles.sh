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

cd ../android

scripts=(update_assets.sh update_assets_yota.sh)
for item in ${scripts[*]}
do
  sh $item
  if [ $? -ne 0  ]; then
    cd $MY_PATH
    echo "Error"
    exit 1 # error
  fi
done

cd $MY_PATH

echo "Done"
exit 0 # ok
