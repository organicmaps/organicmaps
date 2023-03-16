#!/bin/sh
#
#  run_benchmarks.sh
#
#  Run all benchmarks.
#

set -e

for benchmark in @CMAKE_BINARY_DIR@/benchmarks/run_benchmark_*.sh; do
    name=`basename $benchmark`
    echo "Running $name..."
    $benchmark
done

