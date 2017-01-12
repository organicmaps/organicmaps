#!/bin/bash
set -e -u

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
SKIN_GENERATOR="$OMIM_PATH/out/release/skin_generator"
DATA_PATH="$OMIM_PATH/data"

# If skin_generator does not exist then build it
if [ ! -f $SKIN_GENERATOR ];
then
  source "$OMIM_PATH/tools/autobuild/detect_qmake.sh"

  # OS-specific parameters
  if [ "$(uname -s)" == "Darwin" ]; then
    SPEC=${SPEC:-macx-clang}
    PROCESSES=$(sysctl -n hw.ncpu)
  else
    SPEC=${SPEC:-linux-clang-libc++}
    PROCESSES=$(nproc)
  fi

  for project in freetype gflags
  do
    cd "$OMIM_PATH/3party/$project"
    "$QMAKE" $project.pro -r -spec $SPEC CONFIG+=x86_64
    make -j $PROCESSES
  done
  for project in base coding geometry skin_generator
  do
    cd "$OMIM_PATH/$project"
    "$QMAKE" $project.pro -r -spec $SPEC CONFIG+=x86_64
    make -j $PROCESSES
  done
fi

# Helper function to build skin
# Parameter $1 - style type (clear)
# Parameter $2 - style name (dark, light, clear, ...)
# Parameter $3 - resource name (mdpi, hdpi, ...)
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
  STYLE_PATH="$DATA_PATH/styles/$styleType/style-$styleName"
  PNG_PATH="$STYLE_PATH/symbols/png"
  rm -rf "$PNG_PATH" || true
  ln -s "$STYLE_PATH/$resourceName" $PNG_PATH
  # Run sking generator
  if [ $colorCorrection = "true" ]; then
    COLOR_CORR="--colorCorrection true"
  else
    COLOR_CORR=
  fi
  "$SKIN_GENERATOR" --symbolWidth $symbolSize --symbolHeight $symbolSize --symbolsDir "$STYLE_PATH/symbols" \
      --skinName "$DATA_PATH/resources-$resourceName$suffix/basic" --skinSuffix="" $COLOR_CORR
  # Reset environment
  rm -r $PNG_PATH || true
}

# Cleanup
cleanup=(resources-{{6plus,mdpi,hdpi,xhdpi,xxhdpi}{_dark,_clear}})
for item in ${cleanup[*]}
do
  rm -rf "$DATA_PATH/$item" || true
  mkdir "$DATA_PATH/$item"
done

# Build styles

BuildSkin clear  night mdpi   18 false _dark
BuildSkin clear  night hdpi   27 false _dark
BuildSkin clear  night xhdpi  36 false _dark
BuildSkin clear  night xxhdpi 54 false _dark
BuildSkin clear  night 6plus  54 false _dark

BuildSkin clear  clear mdpi   18 false _clear
BuildSkin clear  clear hdpi   27 false _clear
BuildSkin clear  clear xhdpi  36 false _clear
BuildSkin clear  clear xxhdpi 54 false _clear
BuildSkin clear  clear 6plus  54 false _clear
