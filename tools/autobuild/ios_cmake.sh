# Script takes configuration as a parameter
set -e -u -x

MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

if [[ $# < 1 ]]; then
  echo "Usage: $0 <debug|release>"
  exit 1
fi
CONFIGURATION="$1"

source "$MY_PATH/build.sh"
source "$MY_PATH/detect_xcode.sh"

BuildCmake() {
  (
    SHADOW_DIR="$1"
    BUILD_TYPE="$2"

    mkdir -p "$SHADOW_DIR"
    cd "$SHADOW_DIR"
    pwd
    echo "Launching cmake..."
    CC=$CC CXX=$CXX cmake -r "$BUILD_TYPE" -DCMAKE_OSX_ARCHITECTURES=$ARCH -DNO_TESTS=TRUE -DPLATFORM=$PLATFORM "$(StripCygwinPrefix $MY_PATH)/../.."
#    make clean > /dev/null || true
    make -j $(GetCPUCores) VERBOSE=1
  )
}


SDK_ROOT="$(PrintIOSSDKPath "$CONFIGURATION")"
if [[ $? -ne 0 ]]; then
  echo "Is XCode installed? Check tools/autobuild/detect_xcode.sh script"
  exit 1
fi
export SDK_ROOT

FINAL_PATH_COMPONENT="debug"
SHADOW_DIR="$MY_PATH/../../../omim-iphone-cmake"

if [[ $CONFIGURATION == *release* ||  $CONFIGURATION == *production*  ]]; then
  BUILD_TYPE="Release"
  FINAL_PATH_COMPONENT="release"
elif [[ $CONFIGURATION == *debug* || $CONFIGURATION == "simulator" ]]; then
  BUILD_TYPE="Debug"
  FINAL_PATH_COMPONENT="debug"
else
  echo "Unrecognized configuration passed to the script: $CONFIGURATION"
  exit 1
fi

SHADOW_DIR="${SHADOW_DIR}-${FINAL_PATH_COMPONENT}"

# Build libs for each architecture in separate folders
for ARCH in $VALID_ARCHS; do
  if [[ $# > 1 && "$2" == "clean" ]] ; then
    echo "Cleaning $CONFIGURATION configuration..."
    rm -rf "$SHADOW_DIR-$ARCH"
  else
    export BUILD_ARCHITECTURE="$ARCH"

    export CC=clang
    export CXX=clang++
    export CMAKE_PREFIX_PATH=$QT_PATH
    export PLATFORM=iphone-something

    BuildCmake "$SHADOW_DIR-$ARCH/out/$FINAL_PATH_COMPONENT" "$BUILD_TYPE" || ( echo "ERROR while building $CONFIGURATION config"; exit 1 )
  fi
done
