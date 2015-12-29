#!/bin/bash
OPT_DEBUG=
OPT_RELEASE=
OPT_OSRM=
OPT_CLEAN=
while getopts ":cdro" opt; do
  case $opt in
    d)
      OPT_DEBUG=1
      ;;
    r)
      OPT_RELEASE=1
      ;;
    o)
      OPT_OSRM=1
      ;;
    c)
      OPT_CLEAN=1
      ;;
    *)
      echo "This tool builds omim and osrm-backend."
      echo "Usage: $0 [-d] [-r] [-o] [-c]"
      echo
      echo -e "-d\tBuild omim-debug"
      echo -e "-r\tBuild omim-release"
      echo -e "-o\tBuild osrm-backend"
      echo -e "-c\tClean before building"
      echo
      echo "By default release is built. Specify TARGET and OSRM_TARGET if needed."
      exit 1
      ;;
  esac
done

# By default build everything
if [ -z "$OPT_DEBUG$OPT_RELEASE$OPT_OSRM" ]; then
  OPT_RELEASE=1
fi

set -x -u -e

OMIM_PATH="$(cd "${OMIM_PATH:-$(dirname "$0")/../..}"; pwd)"
BOOST_PATH="${BOOST_PATH:-/usr/local/boost_1.54.0}"
DEVTOOLSET_PATH=/opt/rh/devtoolset-2
if [ -d "$DEVTOOLSET_PATH" ]; then
  export MANPATH=
  source "$DEVTOOLSET_PATH/enable"
else
  DEVTOOLSET_PATH=
fi

# Find qmake, prefer qmake-qt5
if [ ! -x "${QMAKE-}" ]; then
  QMAKE=qmake-qt5
  if ! hash "$QMAKE" 2>/dev/null; then
    QMAKE=qmake
  fi
fi

# Find cmake, prefer cmake28
if [ ! -x "${CMAKE-}" ]; then
  CMAKE=cmake28
  if ! hash "$CMAKE" 2>/dev/null; then
    CMAKE=cmake
  fi
fi

# OS-specific parameters
if [ "$(uname -s)" == "Darwin" ]; then
  SPEC=${SPEC:-macx-clang}
  PROCESSES=$(sysctl -n hw.ncpu)
else
  SPEC=${SPEC:-linux-clang-libc++}
  PROCESSES=$(nproc)
fi

# Build one configuration into $TARGET or omim-build-{debug,release}
build_conf()
{
  CONF=$1
  DIRNAME="${TARGET:-$OMIM_PATH/../omim-build-$CONF}"
  [ -d "$DIRNAME" -a -n "$OPT_CLEAN" ] && rm -r "$DIRNAME"

  if [ ! -d "$DIRNAME" ]; then
    mkdir -p "$DIRNAME"
    ln -s "$OMIM_PATH/data" "$DIRNAME/data"
  fi

  (
    export BOOST_INCLUDEDIR="$BOOST_PATH/include"
    cd "$DIRNAME"
    if [ -n "$DEVTOOLSET_PATH" ]; then
      "$QMAKE" "$OMIM_PATH/omim.pro" -spec $SPEC CONFIG+=$CONF ${CONFIG+"CONFIG*=$CONFIG"} \
        "QMAKE_CXXFLAGS *=--gcc-toolchain=$DEVTOOLSET_PATH/root/usr" \
        "QMAKE_LFLAGS *=--gcc-toolchain=$DEVTOOLSET_PATH/root/usr"
    else
      "$QMAKE" "$OMIM_PATH/omim.pro" -spec $SPEC CONFIG+=$CONF ${CONFIG+"CONFIG*=$CONFIG"}
    fi
    make -j $PROCESSES
  )
}

# Build some omim libraries for osrm backend
build_conf_osrm()
{
  CONF=$1
  DIRNAME="$2"
  mkdir -p "$DIRNAME"
  OSPEC="$SPEC"
  [ "$OSPEC" == "linux-clang-libc++" ] && OSPEC=linux-clang

  (
    export BOOST_INCLUDEDIR="$BOOST_PATH/include"
    cd "$DIRNAME"
    if [ -n "$DEVTOOLSET_PATH" ]; then
      "$QMAKE" "$OMIM_PATH/omim.pro" -spec $OSPEC "CONFIG+=$CONF osrm no-tests" \
        "QMAKE_CXXFLAGS *=--gcc-toolchain=$DEVTOOLSET_PATH/root/usr" \
        "QMAKE_LFLAGS *=--gcc-toolchain=$DEVTOOLSET_PATH/root/usr"
    else
      "$QMAKE" "$OMIM_PATH/omim.pro" -spec $OSPEC "CONFIG+=$CONF osrm"
    fi
    make -j $PROCESSES
  )
}

# Build OSRM Backend
build_osrm()
{
  OSRM_OMIM_CONF=$1
  # Making the first letter uppercase for CMake
  OSRM_CONF="$(echo ${OSRM_OMIM_CONF:0:1} | tr '[a-z]' '[A-Z]')${OSRM_OMIM_CONF:1}"
  BACKEND="$OMIM_PATH/3party/osrm/osrm-backend"
  OSRM_TARGET="${OSRM_TARGET:-${TARGET:-$OMIM_PATH/../osrm-backend-$OSRM_OMIM_CONF}}"
  [ -d "$OSRM_TARGET" -a -n "$OPT_CLEAN" ] && rm -r "$OSRM_TARGET"
  mkdir -p "$OSRM_TARGET"
  # First, build omim libraries
  build_conf_osrm $OSRM_OMIM_CONF "$OSRM_TARGET/omim-build"
  OSRM_OMIM_LIBS="omim-build/out/$OSRM_OMIM_CONF"
  (
    cd "$OSRM_TARGET"
    "$CMAKE" "-DBOOST_ROOT=$BOOST_PATH" -DCMAKE_BUILD_TYPE=$OSRM_CONF "-DOMIM_BUILD_PATH=$OSRM_OMIM_LIBS" "$BACKEND"
    make clean
    make
  )
  rm -r "$OSRM_TARGET/$OSRM_OMIM_LIBS"
}

build()
{
  build_conf $1
  [ -n "$OPT_OSRM" ] && build_osrm $1
  return 0
}

[ -n "$OPT_DEBUG" ]   && build debug
[ -n "$OPT_RELEASE" ] && build release
[ -n "$OPT_OSRM" -a -z "$OPT_DEBUG$OPT_RELEASE" ] && build_osrm release
exit 0
