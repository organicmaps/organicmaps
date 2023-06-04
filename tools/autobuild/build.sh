#!/bin/bash
set -e -x -u

MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

source "$MY_PATH/detect_qmake.sh"

# Prints number of cores to stdout
GetCPUCores() {
  case "$OSTYPE" in
    # it's GitBash under Windows
    cygwin)    echo $NUMBER_OF_PROCESSORS
               ;;
    linux-gnu) grep -c ^processor /proc/cpuinfo 2>/dev/null
               ;;
    darwin*)   sysctl -n hw.ncpu
               ;;
    *)         echo "Unsupported platform in $0"
               exit 1
               ;;
  esac
  return 0
}


# Replaces "/cygwin/c" prefix with "c:" one on Windows platform.
# Does nothing under other OS.
# 1st param: path to be modified.
StripCygwinPrefix() {
  if [[ $(GetNdkHost) == "windows-x86_64" ]]; then
    echo "c:`(echo "$1" | cut -c 12-)`"
    return 0
  fi

  echo "$1"
  return 0
}

# 1st param: shadow directory path
# 2nd param: mkspec
# 3rd param: additional qmake parameters
BuildQt() {
  (
    echo "ERROR! The script is obsolete. Rewrite with cmake"
  )
}
