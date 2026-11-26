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

BINARY_NAME=skin_generator_tool
OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
BUILD_DIR="$OMIM_PATH/build"
SKIN_GENERATOR="${SKIN_GENERATOR:-$BUILD_DIR/$BINARY_NAME}"
DATA_PATH="$OMIM_PATH/data"

# cmake rebuilds skin generator binary if necessary.
cmake -S "$OMIM_PATH" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build "$BUILD_DIR" --target "$BINARY_NAME"


# Helper function to build skin
# Parameters: theme, density, symbol size.
function BuildSkin() {
  styleName=$1
  resourceName=$2
  symbolSize=$3

  echo "Building skin for $styleName/$resourceName"
  "$SKIN_GENERATOR" --symbolSize "$symbolSize" --symbolsDir "$DATA_PATH/styles/default/$styleName/symbols" \
      --outputDir "$DATA_PATH/symbols/$resourceName/$styleName"
}

symbols_name=(6plus mdpi hdpi xhdpi xxhdpi xxxhdpi)

# Cleanup
rm -rf "$DATA_PATH"/symbols/*/*/symbols.*

# Build styles

BuildSkin dark  mdpi    18
BuildSkin dark  hdpi    27
BuildSkin dark  xhdpi   36
BuildSkin dark  6plus   43
BuildSkin dark  xxhdpi  54
BuildSkin dark  xxxhdpi 64

BuildSkin light mdpi    18
BuildSkin light hdpi    27
BuildSkin light xhdpi   36
BuildSkin light 6plus   43
BuildSkin light xxhdpi  54
BuildSkin light xxxhdpi 64

for i in "${symbols_name[@]}"; do
  optipng -zc9 -zm8 -zs0 -f0 "$DATA_PATH"/symbols/"${i}"/light/symbols.png
  optipng -zc9 -zm8 -zs0 -f0 "$DATA_PATH"/symbols/"${i}"/dark/symbols.png
done
