#!/bin/sh
#
#  run_benchmark_static_vs_dynamic_index.sh
#

set -e

BENCHMARK_NAME=static_vs_dynamic_index

. @CMAKE_BINARY_DIR@/benchmarks/setup.sh

CMD=$OB_DIR/osmium_benchmark_$BENCHMARK_NAME

for data in $OB_DATA_FILES; do
    filesize=`stat --format="%s" --dereference $data`
    if [ $filesize -lt 500000000 ]; then
        echo "========================"
        $CMD $data
    fi
done

