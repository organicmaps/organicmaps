#!/bin/bash
set -u -e

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
while getopts ":cdrstagjp:" opt; do
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
    a)
      OPT_STANDALONE=1
      ;;
    g)
      OPT_GCC=1
      ;;
    j)
      OPT_COMPILE_DATABASE=1
      CMAKE_CONFIG="${CMAKE_CONFIG:-} -DCMAKE_EXPORT_COMPILE_COMMANDS=YES"
      ;;
    p)
      OPT_PATH="$OPTARG"
      ;;
    *)
      echo "This tool builds omim"
      echo "Usage: $0 [-d] [-r] [-c] [-s] [-t] [-a] [-g] [-j] [-p PATH] [target1 target2 ...]"
      echo
      echo -e "-d\tBuild omim-debug"
      echo -e "-r\tBuild omim-release"
      echo -e "-c\tClean before building"
      echo -e "-s\tSkip desktop app building"
      echo -e "-t\tBuild designer tool (only for MacOS X platform)"
      echo -e "-a\tBuild standalone desktop app (only for MacOS X platform)"
      echo -e "-g\tForce use GCC (only for MacOS X platform)"
      echo -e "-p\tDirectory for built binaries"
      echo -e "-j\tGenerate compile_commands.json"
      echo "By default both configurations is built."
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
    if [ -n "$OPT_STANDALONE" ]; then
      "$CMAKE" "$OMIM_PATH" -DCMAKE_BUILD_TYPE="$CONF" \
      -DBUILD_STANDALONE:bool=True ${CMAKE_CONFIG:-}
    else
      "$CMAKE" "$OMIM_PATH" -DCMAKE_BUILD_TYPE="$CONF" ${CMAKE_CONFIG:-}
    fi
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
  if [ -n "$OPT_COMPILE_DATABASE" ]; then
    cp "$DIRNAME/compile_commands.json" "$OMIM_PATH"
  fi
}

[ -n "$OPT_DEBUG" ]   && build Debug
[ -n "$OPT_RELEASE" ] && build Release
exit 0
