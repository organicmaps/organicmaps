#!/bin/sh
#
#  run_benchmark_index_map.sh
#

set -e

BENCHMARK_NAME=index_map

. @CMAKE_BINARY_DIR@/benchmarks/setup.sh

CMD=$OB_DIR/osmium_benchmark_$BENCHMARK_NAME

#MAPS="sparse_mem_map sparse_mem_table sparse_mem_array sparse_mmap_array sparse_file_array dense_mem_array dense_mmap_array dense_file_array"
MAPS="sparse_mem_map sparse_mem_table sparse_mem_array sparse_mmap_array sparse_file_array"

echo "# file size num mem time cpu_kernel cpu_user cpu_percent cmd options"
for data in $OB_DATA_FILES; do
    filename=`basename $data`
    filesize=`stat --format="%s" --dereference $data`
    for map in $MAPS; do
        for n in $OB_SEQ; do
            $OB_TIME_CMD -f "$filename $filesize $n $OB_TIME_FORMAT" $CMD $data $map 2>&1 >/dev/null | sed -e "s%$DATA_DIR/%%" | sed -e "s%$OB_DIR/%%"
        done
    done
done

