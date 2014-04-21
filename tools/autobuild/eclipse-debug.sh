# Add it to Eclipse MWM Library properties -> Debug configuration -> Build command

set -e -u -x

LOCAL_DIRNAME="$(dirname "$0")"

# We DO NOT clean native C++ libs for faster Eclipse builds
if [[ "$1" != "clean" ]] ; then
  bash $LOCAL_DIRNAME/android.sh debug armeabi
fi

source "$LOCAL_DIRNAME/ndk_helper.sh"

export NDK_PROJECT_PATH="$LOCAL_DIRNAME/../../android"
if [[ "$1" != "clean" ]] ; then
  $(GetNdkRoot)/ndk-build APP_ABI=armeabi V=1 NDK_DEBUG=1 DEBUG=1
else
  $(GetNdkRoot)/ndk-build clean V=1 NDK_DEBUG=1 DEBUG=1
fi
