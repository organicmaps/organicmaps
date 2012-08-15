# Script takes configuration as a parameter and optional clean keyword.
# Possible configurations: debug release production

set -e -u -x

LOCAL_DIRNAME="$(dirname "$0")"
#LOCAL_DIRNAME="${PWD}/$(dirname "$0")"

if [[ $# < 1 ]]; then
  echo "Usage: $0 <debug|release|production|simulator> [clean]"
  exit 1
fi
CONFIGURATION="$1"

source "$LOCAL_DIRNAME/build.sh"
source "$LOCAL_DIRNAME/detect_xcode.sh"

SDK_ROOT="$(PrintIOSSDKPath "$CONFIGURATION")"
if [[ $? -ne 0 ]]; then
  echo "Is XCode installed? Check tools/autobuild/detect_xcode.sh script"
  exit 1
fi
export SDK_ROOT

MKSPEC="$LOCAL_DIRNAME/../mkspecs/iphonedevice-llvm"
QMAKE_PARAMS="CONFIG+=${CONFIGURATION}"
if [[ $CONFIGURATION == "production" ]] ; then
  QMAKE_PARAMS="$QMAKE_PARAMS CONFIG+=release"
fi

SHADOW_DIR_BASE="$LOCAL_DIRNAME/../../../omim-iphone"

if [[ $CONFIGURATION == "simulator" ]]; then
  SHADOW_DIR="${SHADOW_DIR_BASE}sim-debug"
  MKSPEC="$LOCAL_DIRNAME/../mkspecs/iphonesimulator-clang"
else
  SHADOW_DIR="${SHADOW_DIR_BASE}-${CONFIGURATION}"
  MKSPEC="$LOCAL_DIRNAME/../mkspecs/iphonedevice-clang"
fi

if [[ $# > 1 && "$2" == "clean" ]] ; then
  echo "Cleaning $CONFIGURATION configuration..."
  rm -rf "$SHADOW_DIR"
else
  BuildQt "$SHADOW_DIR" "$MKSPEC" "$QMAKE_PARAMS" || ( echo "ERROR while building $CONFIGURATION config"; exit 1 )
fi
