#!/usr/bin/env bash

set -eu

OPT_DEBUG=
OPT_RELEASE=
OPT_CLEAN=
OPT_SKIP_DESKTOP=
OPT_DESIGNER=
OPT_GCC=
OPT_TARGET=
OPT_PATH=
OPT_STANDALONE=
OPT_COMPILE_DATABASE=
OPT_LAUNCH_BINARY=
OPT_NJOBS=
while getopts ":cdrxstagjlpn:" opt; do
  case $opt in
    a) OPT_STANDALONE=1 ;;
    c) OPT_CLEAN=1 ;;
    d) OPT_DEBUG=1 ;;
    x) CMAKE_CONFIG="${CMAKE_CONFIG:-} -DUSE_PCH=YES" ;;
    g) OPT_GCC=1 ;;
    j) OPT_COMPILE_DATABASE=1
       CMAKE_CONFIG="${CMAKE_CONFIG:-} -DCMAKE_EXPORT_COMPILE_COMMANDS=YES"
      ;;
    l) OPT_LAUNCH_BINARY=1 ;;
    n) OPT_NJOBS="$OPTARG"
       CMAKE_CONFIG="${CMAKE_CONFIG:-} -DNJOBS=${OPT_NJOBS}"
      ;;
    p) OPT_PATH="$OPTARG" ;;
    r) OPT_RELEASE=1 ;;
    s) OPT_SKIP_DESKTOP=1
       CMAKE_CONFIG="${CMAKE_CONFIG:-} -DSKIP_DESKTOP=ON"
      ;;
    t) OPT_DESIGNER=1 ;;
    *)
      echo "This tool builds Organic Maps"
      echo "Usage: $0 [-d] [-r] [-c] [-x] [-s] [-t] [-a] [-g] [-j] [-l] [-p PATH] [-n NUM] [target1 target2 ...]"
      echo
      echo "By default both debug and release versions are built in ../omim-build-<buildtype> dir."
      echo
      echo -e "-d  Build debug version"
      echo -e "-r  Build release version"
      echo -e "-x  Use precompiled headers"
      echo -e "-c  Clean before building"
      echo -e "-s  Skip desktop app building"
      echo -e "-t  Build designer tool (only for MacOS X platform)"
      echo -e "-a  Build standalone desktop app (only for MacOS X platform)"
      echo -e "-g  Force use GCC (only for MacOS X platform)"
      echo -e "-p  Directory for built binaries"
      echo -e "-n  Number of parallel processes"
      echo -e "-j  Generate compile_commands.json"
      echo -e "-l  Launches built binary(ies), useful for tests"
      exit 1
      ;;
  esac
done

[ -n "$OPT_DESIGNER" -a -n "$OPT_SKIP_DESKTOP" ] &&
echo "Can't skip desktop and build designer tool simultaneously" &&
exit 2

[ -n "$OPT_STANDALONE" -a -n "$OPT_SKIP_DESKTOP" ] &&
echo "Can't skip desktop and build standalone desktop app simultaneously" &&
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

DEVTOOLSET_PATH=/opt/rh/devtoolset-7
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

  if [ -n "$OPT_GCC" ]; then
    GCC="$(ls /usr/local/bin | grep '^gcc-[6-9][0-9]\?' -m 1)" || true
    GPP="$(ls /usr/local/bin | grep '^g++-[6-9][0-9]\?' -m 1)" || true
    [ -z "$GCC" -o -z "$GPP" ] \
    && echo "Either gcc or g++ is not found. Note, minimal supported gcc version is 6." \
    && exit 2
    CMAKE_CONFIG="${CMAKE_CONFIG:-} -DCMAKE_C_COMPILER=/usr/local/bin/$GCC \
                                    -DCMAKE_CXX_COMPILER=/usr/local/bin/$GPP"
  fi
else
  [ -n "$OPT_DESIGNER" ] \
  && echo "Designer tool supported only on MacOS X platform" && exit 2
  [ -n "$OPT_STANDALONE" ] \
  && echo "Standalone desktop app supported only on MacOS X platform" && exit 2
  PROCESSES=$(nproc)
fi

if [ -n "$OPT_NJOBS" ]; then
  PROCESSES="$OPT_NJOBS"
fi

build()
{
  local MAKE_COMMAND=$(which ninja)
  local CMAKE_GENERATOR=
  if [ -z "$MAKE_COMMAND" ]; then
    echo "Ninja is not found, using make instead"
    MAKE_COMMAND="make -j $PROCESSES"
  else
    CMAKE_GENERATOR=-GNinja
  fi

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
  if [ -z "$OPT_DESIGNER" ]; then
    if [ -n "$OPT_STANDALONE" ]; then
      "$CMAKE" "$CMAKE_GENERATOR" "$OMIM_PATH" -DCMAKE_BUILD_TYPE="$CONF" \
      -DBUILD_STANDALONE:bool=True ${CMAKE_CONFIG:-}
    else
      "$CMAKE" "$CMAKE_GENERATOR" "$OMIM_PATH" -DCMAKE_BUILD_TYPE="$CONF" ${CMAKE_CONFIG:-}
    fi
    echo ""
    $MAKE_COMMAND $OPT_TARGET
    if [ -n "$OPT_TARGET" ] && [ -n "$OPT_LAUNCH_BINARY" ]; then
      for target in $OPT_TARGET; do
        "$DIRNAME/$target"
      done
    fi
  else
    "$CMAKE" "$CMAKE_GENERATOR" "$OMIM_PATH" -DCMAKE_BUILD_TYPE="$CONF" \
    -DBUILD_DESIGNER:bool=True ${CMAKE_CONFIG:-}
    $MAKE_COMMAND package
  fi
  if [ -n "$OPT_COMPILE_DATABASE" ]; then
    cp "$DIRNAME/compile_commands.json" "$OMIM_PATH"
  fi
}

[ -n "$OPT_DEBUG" ]   && build Debug
[ -n "$OPT_RELEASE" ] && build Release
exit 0
