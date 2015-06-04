#!/bin/bash
set -e -u -x

function BuildDrawingRules() {
  styleName=$1
  suffix=$2
  echo "Building drawing rules for style $styleName"
  # Cleanup
  rm ../../data/drules_proto$suffix.bin || true
  rm ../../data/drules_proto$suffix.txt || true
  # Run script to build style
  python ../kothic/libkomwm.py -s ../../data/styles/style-$styleName/style.mapcss \
                               -o ../../data/drules_proto$suffix
  res=$?
  # Check result
  if [ $res -ne 0  ]; then
    echo "Error"
    exit 1 # error
  fi
}

# Cleanup
cleanup=(classificator.txt types.txt visibility.txt)
for item in ${cleanup[*]}
do
  rm ../../data/$item || true
done

# Building drawing rules light
BuildDrawingRules light ""

# Building drawing rules dark
BuildDrawingRules dark "_dark"

echo "Done"
exit 0 # ok
