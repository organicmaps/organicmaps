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

ls "$OSRM_DIR"/*.osrm | parallel -t -v "V=`basename -s .osrm {}`; $GENERATOR_TOOL --osrm_file_name={} --data_path=$DATA_DIR --output={/.}"
