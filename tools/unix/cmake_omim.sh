#!/usr/bin/env bash

set -u -e -x

OPT_DEBUG=
OPT_RELEASE=1
OPT_OSRM=
OPT_CLEAN=
VERBOSE=
while getopts ":cdrov" opt; do
  case $opt in
    d)
      OPT_DEBUG=1
      OPT_RELEASE=
      ;;
    r)
      OPT_RELEASE=1
      ;;
    o)
#      OPT_OSRM=1
        echo "OSRM build is not supported yet, try again later"
        exit 1
      ;;
    c)
      OPT_CLEAN=1
      ;;
    v)
      VERBOSE=1
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

OMIM_PATH="$(cd "${OMIM_PATH:-$(dirname "$0")/../..}"; pwd)"

check_private_h()
{
    if ! grep "DEFAULT_URLS_JSON" "$OMIM_PATH/private.h" >/dev/null 2>/dev/null; then
      echo "Please run $OMIM_PATH/configure.sh"
      exit 2
    fi
}

set_platform_dependent_options()
{
  if [ "$(uname -s)" == "Darwin" ]; then
    PROCESSES=$(sysctl -n hw.ncpu)
  else
    PROCESSES=$(nproc)
    export CC=clang
    export CXX=clang++
  fi
}

build_conf()
{
  CONF=$1
  POSTFIX=$(echo "${CONF}" | tr '[:upper:]' '[:lower:]')
  echo $POSTFIX
  DIRNAME="${TARGET:-$OMIM_PATH/../omim-build-$POSTFIX}"
  [ -d "$DIRNAME" -a -n "$OPT_CLEAN" ] && rm -r "$DIRNAME"

  if [ ! -d "$DIRNAME" ]; then
    mkdir -p "$DIRNAME"
    ln -s "$OMIM_PATH/data" "$DIRNAME/data"
  fi

  TARGET="$DIRNAME/out/$POSTFIX"
  mkdir -p $TARGET

  cd "$TARGET"
  cmake "$OMIM_PATH" -DCMAKE_BUILD_TYPE=$CONF
  if [ $VERBOSE ]; then
    make -j $PROCESSES VERBOSE=1
  else
    make -j $PROCESSES
  fi
}

get_os_dependent_option() #(for_mac_os, for_linux)
{
  if [ "$(uname -s)" == "Darwin" ]; then
    return $($1)
  else
    return $($2)
  fi

}

check_private_h
set_platform_dependent_options
echo $PROCESSES

[ -n "$OPT_DEBUG" ]   && build_conf Debug
[ -n "$OPT_RELEASE" ] && build_conf Release

exit 0
