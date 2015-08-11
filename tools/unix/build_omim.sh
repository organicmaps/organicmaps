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
      echo "By default everything is built. Specify TARGET for omim-* if needed."
      exit 1
      ;;
  esac
done

# By default build everything
if [ -z "$OPT_DEBUG$OPT_RELEASE$OPT_OSRM" ]; then
  OPT_DEBUG=1
  OPT_RELEASE=1
  OPT_OSRM=1
fi

set -x -u -e

BOOST_PATH="${BOOST_PATH:-/usr/local/boost_1.54.0}"
DEVTOOLSET_PATH=/opt/rh/devtoolset-2
if [ -d "$DEVTOOLSET_PATH" ]; then
  . "$DEVTOOLSET_PATH/enable"
else
  DEVTOOLSET_PATH=
fi
OMIM_PATH="$(cd "${OMIM_PATH:-$(dirname "$0")/../..}"; pwd)"
PRO_FILE="$OMIM_PATH/${PROJECT:-omim}.pro"
export MANPATH=""
QMAKE=qmake-qt5
if ! hash "$QMAKE" 2>/dev/null; then
  QMAKE=qmake
fi
CMAKE=cmake28
if ! hash "$CMAKE" 2>/dev/null; then
  CMAKE=cmake
fi
PROCESSES=$(($(nproc) / 2))

build_conf()
{
  CONF=$1
  DIRNAME="${TARGET:-$OMIM_PATH/..}/omim-build-$CONF"
  [ -d "$DIRNAME" -a -n "$OPT_CLEAN" ] && rm -r "$DIRNAME"

  if [ ! -d "$DIRNAME" ]; then
    mkdir "$DIRNAME"
    ln -s "$OMIM_PATH/data" "$DIRNAME/data"
  fi

  (
    export BOOST_INCLUDEDIR="$BOOST_PATH/include"
    cd "$DIRNAME"
    if [ -n "$DEVTOOLSET_PATH" ]; then
      "$QMAKE" -r "$PRO_FILE" -spec linux-clang CONFIG+=$CONF \
        "QMAKE_CXXFLAGS *=--gcc-toolchain=$DEVTOOLSET_PATH/root/usr" \
        "QMAKE_LFLAGS *=--gcc-toolchain=$DEVTOOLSET_PATH/root/usr"
    else
      "$QMAKE" -r "$PRO_FILE" -spec linux-clang CONFIG+=$CONF
    fi
    make -j $PROCESSES
  )
}

[ -n "$OPT_DEBUG" ]   && build_conf debug
[ -n "$OPT_RELEASE" ] && build_conf release

if [ -n "$OPT_OSRM" ]; then
  OSRM_TARGET="${OSRM_TARGET:-$OMIM_PATH/3party/osrm/osrm-backend/build}"
  [ -d "$OSRM_TARGET" -a -n "$OPT_CLEAN" ] && rm -r "$OSRM_TARGET"
  mkdir -p "$OSRM_TARGET"
  (
    cd "$OSRM_TARGET"
    "$CMAKE" "-DBOOST_ROOT=$BOOST_PATH" ..
    make clean
    make
  )
fi
