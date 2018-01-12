#!/bin/bash
set -u -e

OPT_DEBUG=
OPT_RELEASE=
OPT_CLEAN=
OPT_SKIP_DESKTOP=
OPT_DESIGNER=
OPT_TARGET=
OPT_PATH=

while getopts ":cdrstp:" opt; do
  case $opt in
    d)
      OPT_DEBUG=1
      ;;
    r)
      OPT_RELEASE=1
      ;;
    c)
      OPT_CLEAN=1
      ;;
    s)
      OPT_SKIP_DESKTOP=1
      CMAKE_CONFIG="${CMAKE_CONFIG:-} -DSKIP_DESKTOP=ON"
      ;;
    t)
      OPT_DESIGNER=1
      ;;
    p)
      OPT_PATH="$OPTARG"
      ;;
    *)
      echo "This tool builds omim"
      echo "Usage: $0 [-d] [-r] [-c] [-s] [-g] [-p PATH] [target1 target2 ...]"
      echo
      echo -e "-d\tBuild omim-debug"
      echo -e "-r\tBuild omim-release"
      echo -e "-c\tClean before building"
      echo -e "-s\tSkip desktop app building"
      echo -e "-t\tBuild designer tool (only for MacOS X platform)"
      echo -e "-p\tDirectory for built binaries"
      echo "By default both configurations is built."
      exit 1
      ;;
  esac
done

[ -n "$OPT_DESIGNER" -a -n "$OPT_SKIP_DESKTOP" ] &&
echo "Can't skip desktop and build designer tool simultaneously" &&
exit 2

OPT_TARGET=${@:$OPTIND}

# By default build everything
if [ -z "$OPT_DEBUG$OPT_RELEASE" ]; then
  OPT_DEBUG=1
  OPT_RELEASE=1
fi

OMIM_PATH="$(cd "${OMIM_PATH:-$(dirname "$0")/../..}"; pwd)"
if ! grep "DEFAULT_URLS_JSON" "$OMIM_PATH/private.h" >/dev/null 2>/dev/null; then
  echo "Please run $OMIM_PATH/configure.sh"
  exit 2
fi

DEVTOOLSET_PATH=/opt/rh/devtoolset-6
if [ -d "$DEVTOOLSET_PATH" ]; then
  export MANPATH=
  source "$DEVTOOLSET_PATH/enable"
else
  DEVTOOLSET_PATH=
fi

# Find cmake
source "$OMIM_PATH/tools/autobuild/detect_cmake.sh"

# OS-specific parameters
if [ "$(uname -s)" == "Darwin" ]; then
  PROCESSES=$(sysctl -n hw.ncpu)
else
  [ -n "$OPT_DESIGNER" ] \
  && echo "Designer tool supported only on MacOS X platform" && exit 2
  PROCESSES=$(nproc)
  # Let linux version be built with gcc
  CMAKE_CONFIG="${CMAKE_CONFIG:-} -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++"
fi

build()
{
  CONF=$1
  if [ -n "$OPT_PATH" ]; then
    DIRNAME="$OPT_PATH/omim-build-$(echo "$CONF" | tr '[:upper:]' '[:lower:]')"
  else
    DIRNAME="$OMIM_PATH/../omim-build-$(echo "$CONF" | tr '[:upper:]' '[:lower:]')"
  fi
  [ -d "$DIRNAME" -a -n "$OPT_CLEAN" ] && rm -r "$DIRNAME"
  if [ ! -d "$DIRNAME" ]; then
    mkdir -p "$DIRNAME"
    ln -s "$OMIM_PATH/data" "$DIRNAME/data"
  fi
  cd "$DIRNAME"
  TMP_FILE="build_error.log"
  if [ -z "$OPT_DESIGNER" ]; then
    "$CMAKE" "$OMIM_PATH" -DCMAKE_BUILD_TYPE="$CONF" ${CMAKE_CONFIG:-}
    echo ""
    if ! make $OPT_TARGET -j $PROCESSES 2> "$TMP_FILE"; then
      echo '--------------------'
      cat "$TMP_FILE"
      exit 1
    fi
  else
    "$CMAKE" "$OMIM_PATH" -DCMAKE_BUILD_TYPE="$CONF" \
    -DBUILD_DESIGNER:bool=True ${CMAKE_CONFIG:-}
    if ! make package -j $PROCESSES 2> "$TMP_FILE"; then
      echo '--------------------'
      cat "$TMP_FILE"
      exit 1
    fi
  fi
}

[ -n "$OPT_DEBUG" ]   && build Debug
[ -n "$OPT_RELEASE" ] && build Release
exit 0
