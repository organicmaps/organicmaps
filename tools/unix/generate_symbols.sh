#!/bin/bash
set -e -u -x

MY_PATH=`pwd`
BINARY_PATH="$MY_PATH/../../out/release/skin_generator"
DATA_PATH="$MY_PATH/../../data"

# If skin_generator does not exist then build it
if [ ! -f $BINARY_PATH ];
then
  projects=(freetype gflags)
  for project in ${projects[*]}
  do
    cd $MY_PATH/../../3party/$project
    qmake $project.pro -r -spec macx-clang CONFIG+=x86_64
    make
  done
  projects=(base coding geometry skin_generator)
  for project in ${projects[*]}
  do
    cd $MY_PATH/../../$project
    qmake $project.pro -r -spec macx-clang CONFIG+=x86_64
    make
  done
  cd $MY_PATH
fi

# Helper function to build skin
# Parameter $1 - style name (dark, light, yota, ...)
# Parameter $2 - resource name (ldpi, mdpi, hdpi, ...)
# Parameter $3 - symbol size
# Parameter $4 - does color correction required
function BuildSkin() {
  styleName=$1
  resourceName=$2
  symbolSize=$3
  colorCorrection=$4
  suffix=$5
  echo "Building skin for $styleName/$resourceName"
  # Set environment
  PNG_PATH="$DATA_PATH/styles/style-$styleName/symbols/png"
  rm -r $PNG_PATH || true
  ln -s "$DATA_PATH/styles/style-$styleName/$resourceName" $PNG_PATH
  # Run sking generator
  if [ $colorCorrection = "true" ];
  then
    "$BINARY_PATH" --symbolWidth $symbolSize --symbolHeight $symbolSize \
        --symbolsDir "$DATA_PATH/styles/style-$styleName/symbols" \
        --skinName "$DATA_PATH/resources-$resourceName$suffix/basic" --skinSuffix="" \
        --colorCorrection true
  else
    "$BINARY_PATH" --symbolWidth $symbolSize --symbolHeight $symbolSize \
        --symbolsDir "$DATA_PATH/styles/style-$styleName/symbols" \
        --skinName "$DATA_PATH/resources-$resourceName$suffix/basic" --skinSuffix=""
  fi
  res=$?
  # Reset environment
  rm -r $PNG_PATH || true
  # Check result
  if [ $res -ne 0 ];
  then
    echo "Error"
    exit 1 # error
  fi
}

# Cleanup
cleanup=(resources-yota resources-6plus resources-ldpi resources-mdpi resources-hdpi resources-xhdpi resources-xxhdpi \
         resources-6plus_dark resources-ldpi_dark resources-mdpi_dark resources-hdpi_dark resources-xhdpi_dark resources-xxhdpi_dark)
for item in ${cleanup[*]}
do
  rm -rf ../../data/$item || true
  mkdir ../../data/$item
done

# Build style yota
BuildSkin yota yota 19 true ""

# Build style light
BuildSkin light ldpi 16 false ""
BuildSkin light mdpi 16 false ""
BuildSkin light hdpi 24 false ""
BuildSkin light xhdpi 36 false ""
BuildSkin light xxhdpi 48 false ""
BuildSkin light 6plus 38 false ""

# Build style dark
BuildSkin dark ldpi 16 false "_dark"
BuildSkin dark mdpi 16 false "_dark"
BuildSkin dark hdpi 24 false "_dark"
BuildSkin dark xhdpi 36 false "_dark"
BuildSkin dark xxhdpi 48 false "_dark"
BuildSkin dark 6plus 38 false "_dark"

# Success
echo "Done"
exit 0 # ok
