#!/bin/sh
#
#  setup.sh
#

if [ -z $DATA_DIR ]; then
    echo "Please set DATA_DIR environment variable before running benchmark"
    exit 1
fi

OB_DIR=@CMAKE_BINARY_DIR@/benchmarks

OB_RUNS=3
OB_SEQ=`seq -s' ' 1 $OB_RUNS`

OB_TIME_CMD=/usr/bin/time
OB_TIME_FORMAT="%M %e %S %U %P %C"

OB_DATA_FILES=`find -L $DATA_DIR -mindepth 1 -maxdepth 1 -type f | sort`

echo "BENCHMARK: $BENCHMARK_NAME"
echo "---------------------"
echo "CPU:"
grep '^model name' /proc/cpuinfo | tail -1
grep '^cpu MHz' /proc/cpuinfo | tail -1
grep '^cpu cores' /proc/cpuinfo | tail -1
grep '^siblings' /proc/cpuinfo | tail -1

echo "---------------------"
echo "MEMORY:"
free
echo "---------------------"
echo "RESULTS:"

