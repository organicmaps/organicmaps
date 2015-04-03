#!/bin/bash

set -e -u -x

if [[ `hostname` == "Scout" ]]; then
  DATA_DIR=/media/ssd/osm/omim/data
  OSRM_DIR=~/toolchain/v2/data/planet-split
  GENERATOR_TOOL=/media/ssd/osm/omim-build-release/out/release/generator_tool
else
  DATA_DIR=~/omim/data
  OSRM_DIR=~/planet-split
  GENERATOR_TOOL=~/omim-build-release/out/release/generator_tool
fi

NUM_INSTANCES=8

OSRM_LIST=${OSRM_LIST-$(ls -1 $OSRM_DIR/*.osrm | xargs -d "\n" basename -s .osrm)}

echo "$OSRM_LIST" |  xargs -d "\n" -P $NUM_INSTANCES -I % $GENERATOR_TOOL --make_routing --make_cross_section --osrm_file_name=$OSRM_DIR/%.osrm --data_path=$DATA_DIR --output=%

