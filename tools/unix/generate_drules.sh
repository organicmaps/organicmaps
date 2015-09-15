#!/bin/bash
set -e -u -x

export PYTHONDONTWRITEBYTECODE=1

DATA_PATH="$(dirname "$0")/../../data"

function BuildDrawingRules() {
  styleType=$1
  styleName=$2
  suffix=${3-}
  echo "Building drawing rules for style $styleName"
  # Cleanup
  rm $DATA_PATH/drules_proto$suffix.bin || true
  rm $DATA_PATH/drules_proto$suffix.txt || true
  # Run script to build style
  python ../kothic/libkomwm.py -s $DATA_PATH/styles/$styleType/style-$styleName/style.mapcss \
                               -o $DATA_PATH/drules_proto$suffix
}

# Cleanup
cleanup=(classificator.txt types.txt visibility.txt)
for item in ${cleanup[*]}
do
  rm $DATA_PATH/$item || true
done

# Building drawing rules
BuildDrawingRules clear  clear _clear
BuildDrawingRules clear  night _dark
BuildDrawingRules legacy light

echo "Done"
exit 0 # ok
