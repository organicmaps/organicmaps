# Script builds only C++ native libs. To build also jni part see another script: eclipse[*].sh

set -e -u -x

MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

if [[ $# < 1 ]]; then
  echo "Usage: $0 <debug|release|production> [armeabi-v7a-hard|x86] [android-PLATFORM_NUMBER]"
  exit 1
fi
CONFIGURATION="$1"

source "$MY_PATH/build.sh"
source "$MY_PATH/ndk_helper.sh"

MKSPEC="$MY_PATH/../mkspecs/android-clang"
QMAKE_PARAMS="CONFIG+=${CONFIGURATION}"
SHADOW_DIR_BASE="$MY_PATH/../../../omim-android-drape"

# Try to read ndk root path from android/local.properties file
export NDK_ROOT=$(GetNdkRoot) || ( echo "Can't read NDK root path from android/local.properties"; exit 1 )
export NDK_HOST=$(GetNdkHost) || ( echo "Can't get your OS type, please check tools/autobuild/ndk_helper.sh script"; exit 1 )
if [[ $# > 2 ]] ; then
  export NDK_PLATFORM=$3
fi

if [[ $# > 1 ]] ; then
  NDK_ABI_LIST=$2
else
  NDK_ABI_LIST=(armeabi-v7a-hard x86)
fi


for abi in "${NDK_ABI_LIST[@]}"; do
  SHADOW_DIR="${SHADOW_DIR_BASE}-${CONFIGURATION}-${abi}"
  export NDK_ABI="$abi"
  BuildQt "$SHADOW_DIR" "$MKSPEC" "$QMAKE_PARAMS" 1>&2 || ( echo "ERROR while building $abi config"; exit 1 )
done
