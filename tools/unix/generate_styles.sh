#!/bin/bash
set -e -u -x

MY_PATH=`pwd`

sh ./generate_symbols.sh

sh ./generate_drules.sh

cd ../android

scripts=(update_assets.sh update_assets_yota.sh)
for item in ${scripts[*]}
do
  sh $item
done

cd $MY_PATH

echo "Done"
exit 0 # ok
