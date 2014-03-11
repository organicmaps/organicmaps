# Script takes configuration as a parameter and optional clean keyword.
# Possible configurations: debug release production

set -e -u -x

MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

if [[ $# < 1 ]]; then
  echo "Usage: $0 <debug|release|production|simulator|simulator-release> [clean]"
  exit 1
fi
CONFIGURATION="$1"

source "$MY_PATH/build.sh"
source "$MY_PATH/detect_xcode.sh"

SDK_ROOT="$(PrintIOSSDKPath "$CONFIGURATION")"
if [[ $? -ne 0 ]]; then
  echo "Is XCode installed? Check tools/autobuild/detect_xcode.sh script"
  exit 1
fi
export SDK_ROOT

SHADOW_DIR="$MY_PATH/../../../omim-iphone"

if [[ $CONFIGURATION == *production* ]]; then
  QMAKE_PARAMS="CONFIG+=production CONFIG+=release"
  SHADOW_DIR="${SHADOW_DIR}-production"
elif [[ $CONFIGURATION == *release* ]]; then
  QMAKE_PARAMS="CONFIG+=release"
  SHADOW_DIR="${SHADOW_DIR}-release"
elif [[ $CONFIGURATION == *debug* || $CONFIGURATION == "simulator" ]]; then
  QMAKE_PARAMS="CONFIG+=debug"
  SHADOW_DIR="${SHADOW_DIR}-debug"
else
  echo "Unrecognized configuration passed to the script: $CONFIGURATION"
  exit 1
fi

if [[ $CONFIGURATION == *simulator* ]]; then
  MKSPEC="$MY_PATH/../mkspecs/iphonesimulator"
else
  MKSPEC="$MY_PATH/../mkspecs/iphonedevice"
fi

if [[ $GCC_VERSION == *clang* ]]; then
  MKSPEC="${MKSPEC}-clang"
else
  MKSPEC="${MKSPEC}-llvm"
fi

# Build libs for each architecture in separate folders
for ARCH in $VALID_ARCHS; do
  if [[ $# > 1 && "$2" == "clean" ]] ; then
    echo "Cleaning $CONFIGURATION configuration..."
    rm -rf "$SHADOW_DIR-$ARCH"
  else
    # pass build architecture to qmake as an environment variable, see mkspecs/iphone*/qmake.conf
    export BUILD_ARCHITECTURE="$ARCH"
    BuildQt "$SHADOW_DIR-$ARCH" "$MKSPEC" "$QMAKE_PARAMS" || ( echo "ERROR while building $CONFIGURATION config"; exit 1 )
  fi
done
