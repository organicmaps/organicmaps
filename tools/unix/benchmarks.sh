#!/bin/bash

set -e -u

# displays usage and exits
function Usage {
  echo ''
  echo "Usage: $0 <file without extension>"
  exit 0
}

########## ENTRY POINT ###########

if [ $# -lt 1 ]; then
  Usage
fi

# trying to locate benchmark tool
SCRIPT_DIR=`dirname $0`
BENCHMARK_TOOL="$SCRIPT_DIR/../../../omim-build-release/out/release/benchmark_tool"
FILE=$1

if [ ! -f $BENCHMARK_TOOL ]; then
  echo "Can't open $BENCHMARK_TOOL"
  exit -1
fi

# build fresh data
echo "**************************"
echo "Starting benchmarking $FILE on `date`"
echo "HEAD commit:"
echo `git --git-dir=$SCRIPT_DIR/../../.git log -1`

echo "`$BENCHMARK_TOOL -input=$FILE.mwm -print_scales`"
if [[ $FILE == World* ]]; then
  SCALES="0 1 2 3 4 5 6 7 8 9"
else
  SCALES="10 11 12 13 14 15 16 17"
fi
for SCALE in $SCALES; do
  echo -n "Scale $SCALE: "
  $BENCHMARK_TOOL -lowS=$SCALE -highS=$SCALE -input=$FILE.mwm
done
