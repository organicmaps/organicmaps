#!/usr/bin/env bash
set -euo pipefail

if ! command -v optipng &> /dev/null
then
    echo -e "\033[1;31moptipng could not be found"
    if [[ $OSTYPE == 'darwin'* ]]; then
       echo 'run command'
       echo 'brew install optipng'
       echo 'to install it'
       exit
    fi
    echo 'take a look to http://optipng.sourceforge.net/'
    exit
fi

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
OUT_PATH="$OMIM_PATH/out/release"
SKIN_GENERATOR="${SKIN_GENERATOR:-$OUT_PATH/skin_generator_tool}"
DATA_PATH="$OMIM_PATH/data"

# If skin_generator does not exist then build it
if [ ! -f "$SKIN_GENERATOR" ];
then
  source "$OMIM_PATH/tools/autobuild/detect_cmake.sh"
  # OS-specific parameters
  if [ "$(uname -s)" == "Darwin" ]; then
    PROCESSES=$(sysctl -n hw.ncpu)
  else
    PROCESSES=$(nproc)
  fi
  mkdir -p "$OUT_PATH"
  pushd "$OUT_PATH" > /dev/null
  "$CMAKE" "$OMIM_PATH" -DSKIP_TESTS:bool=true
  make skin_generator_tool -j$PROCESSES
  popd > /dev/null
fi

# Helper function to build skin
# Parameter $1 - style type (clear)
# Parameter $2 - style name (dark, light, clear, ...)
# Parameter $3 - resource name (mdpi, hdpi, ...)
# Parameter $4 - symbol size
# Parameter $5 - style suffix (none, _dark, _clear)
# Parameter $6 - symbols folder (symbols)
# Parameter $7 - symbols suffix (none, -ad)
function BuildSkin() {
  styleType=$1
  styleName=$2
  resourceName=$3
  symbolSize=$4
  suffix=$5
  symbolsFolder=$6
  symbolsSuffix=${7-}

  echo "Building skin for $styleName/$resourceName"
  # Set environment
  STYLE_PATH="$DATA_PATH/styles/$styleType/style-$styleName"
  PNG_PATH="$STYLE_PATH/symbols$symbolsSuffix/png"
  rm -rf "$PNG_PATH" || true
  ln -s "$STYLE_PATH/$resourceName$symbolsSuffix" "$PNG_PATH"
  # Run skin generator
  "$SKIN_GENERATOR" --symbolWidth $symbolSize --symbolHeight $symbolSize --symbolsDir "$STYLE_PATH/$symbolsFolder" \
      --skinName "$DATA_PATH/resources-$resourceName$suffix/basic" --skinSuffix="$symbolsSuffix"
  # Reset environment
  rm -r "$PNG_PATH" || true
}

# Cleanup
cleanup=(resources-{{6plus,mdpi,hdpi,xhdpi,xxhdpi,xxxhdpi}{_dark,_clear}})
for item in ${cleanup[*]}
do
  rm -rf "$DATA_PATH/$item" || true
  mkdir "$DATA_PATH/$item"
done

# Build styles

BuildSkin clear  night mdpi    18 _dark symbols
BuildSkin clear  night hdpi    27 _dark symbols
BuildSkin clear  night xhdpi   36 _dark symbols
BuildSkin clear  night xxhdpi  54 _dark symbols
BuildSkin clear  night 6plus   54 _dark symbols
BuildSkin clear  night xxxhdpi 64 _dark symbols

BuildSkin clear  clear mdpi    18 _clear symbols
BuildSkin clear  clear hdpi    27 _clear symbols
BuildSkin clear  clear xhdpi   36 _clear symbols
BuildSkin clear  clear xxhdpi  54 _clear symbols
BuildSkin clear  clear 6plus   54 _clear symbols
BuildSkin clear  clear xxxhdpi 64 _clear symbols

rm -rf "$OMIM_PATH"/data/resources-{*}

rm -rf "$OMIM_PATH"/data/resources-*_design

for i in mdpi hdpi xhdpi xxhdpi xxxhdpi 6plus; do
  optipng -zc9 -zm8 -zs0 -f0 "$OMIM_PATH"/data/resources-${i}_clear/symbols.png
  optipng -zc9 -zm8 -zs0 -f0 "$OMIM_PATH"/data/resources-${i}_dark/symbols.png
done

for i in mdpi hdpi xhdpi xxhdpi xxxhdpi 6plus; do
  cp -r "$OMIM_PATH"/data/resources-${i}_clear/ "$OMIM_PATH"/data/resources-${i}_design/
done
