# Script takes configuration as a parameter and optional clean keyword.
# Possible configurations: debug release production

set -e -u -x

LOCAL_DIRNAME="${PWD}/$(dirname "$0")"

if [[ $# < 1 ]]; then
  echo "Usage: $0 <debug|release|production> [clean]"
  exit 1
fi
CONFIGURATION="$1"


source "$LOCAL_DIRNAME/build.sh"

MKSPEC="$LOCAL_DIRNAME/../mkspecs/android-g++"
QMAKE_PARAMS="CONFIG+=${CONFIGURATION}"
SHADOW_DIR_BASE="$LOCAL_DIRNAME/../../../omim-android"

export NDK_HOST=darwin-x86
export NDK_ROOT=/Developer/android-ndk-r7
NDK_ABI_TO_BUILD=(armeabi armeabi-v7a)

for abi in "${NDK_ABI_TO_BUILD[@]}"; do
  SHADOW_DIR="${SHADOW_DIR_BASE}-${CONFIGURATION}-${abi}"
  if [[ $# > 1 && "$2" == "clean" ]] ; then
    echo "Cleaning $CONFIGURATION-$abi configuration..."
    rm -rf "$SHADOW_DIR"
  else
    export NDK_ABI="$abi"
    BuildQt "$SHADOW_DIR" "$MKSPEC" "$QMAKE_PARAMS" || ( echo "ERROR while building $abi config"; exit 1 )
  fi
done
