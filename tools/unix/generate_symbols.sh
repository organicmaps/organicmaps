#!/bin/bash
set -e -u -x

MY_PATH=`pwd`
BINARY_PATH="$MY_PATH/../../out/release/skin_generator"
DATA_PATH="$MY_PATH/../../data"
DETECT_QMAKE="$MY_PATH/../autobuild/detect_qmake.sh"

source "$DETECT_QMAKE"
QMAKE="$(PrintQmakePath)" || ( echo "ERROR: qmake was not found, please add it to your PATH or into the tools/autobuild/detect_qmake.sh"; exit 1 )

# If skin_generator does not exist then build it
if [ ! -f $BINARY_PATH ];
then
  projects=(freetype gflags)
  for project in ${projects[*]}
  do
    cd $MY_PATH/../../3party/$project
    "$QMAKE" $project.pro -r -spec macx-clang CONFIG+=x86_64
    make
  done
  projects=(base coding geometry skin_generator)
  for project in ${projects[*]}
  do
    cd $MY_PATH/../../$project
    "$QMAKE" $project.pro -r -spec macx-clang CONFIG+=x86_64
    make
  done
  cd $MY_PATH
fi

# Helper function to build skin
# Parameter $1 - style type (legacy, clear)
# Parameter $2 - style name (dark, light, yota, clear, ...)
# Parameter $3 - resource name (ldpi, mdpi, hdpi, ...)
# Parameter $4 - symbol size
# Parameter $5 - does color correction required
# Parameter $6 - style suffix (none, _dark, _clear)
function BuildSkin() {
  styleType=$1
  styleName=$2
  resourceName=$3
  symbolSize=$4
  colorCorrection=$5
  suffix=${6-}
  echo "Building skin for $styleName/$resourceName"
  # Set environment
  PNG_PATH="$DATA_PATH/styles/$styleType/style-$styleName/symbols/png"
  rm -r $PNG_PATH || true
  ln -s "$DATA_PATH/styles/$styleType/style-$styleName/$resourceName" $PNG_PATH
  # Run sking generator
  if [ $colorCorrection = "true" ];
  then
    "$BINARY_PATH" --symbolWidth $symbolSize --symbolHeight $symbolSize \
        --symbolsDir "$DATA_PATH/styles/$styleType/style-$styleName/symbols" \
        --skinName "$DATA_PATH/resources-$resourceName$suffix/basic" --skinSuffix="" \
        --colorCorrection true
  else
    "$BINARY_PATH" --symbolWidth $symbolSize --symbolHeight $symbolSize \
        --symbolsDir "$DATA_PATH/styles/$styleType/style-$styleName/symbols" \
        --skinName "$DATA_PATH/resources-$resourceName$suffix/basic" --skinSuffix=""
  fi
  # Reset environment
  rm -r $PNG_PATH || true
}

# Cleanup
cleanup=(resources-{yota,{6plus,ldpi,mdpi,hdpi,xhdpi,xxhdpi}{,_dark,_clear}})
for item in ${cleanup[*]}
do
  rm -rf ../../data/$item || true
  mkdir ../../data/$item
done

# Build styles

BuildSkin legacy yota  yota   19 true

BuildSkin legacy light ldpi   18 false
BuildSkin legacy light mdpi   18 false
BuildSkin legacy light hdpi   27 false
BuildSkin legacy light xhdpi  36 false
BuildSkin legacy light xxhdpi 54 false
BuildSkin legacy light 6plus  54 false

BuildSkin clear  night ldpi   18 false _dark
BuildSkin clear  night mdpi   18 false _dark
BuildSkin clear  night hdpi   27 false _dark
BuildSkin clear  night xhdpi  36 false _dark
BuildSkin clear  night xxhdpi 54 false _dark
BuildSkin clear  night 6plus  54 false _dark

BuildSkin clear  clear ldpi   18 false _clear
BuildSkin clear  clear mdpi   18 false _clear
BuildSkin clear  clear hdpi   27 false _clear
BuildSkin clear  clear xhdpi  36 false _clear
BuildSkin clear  clear xxhdpi 54 false _clear
BuildSkin clear  clear 6plus  54 false _clear

# Success
echo "Done"
exit 0 # ok
