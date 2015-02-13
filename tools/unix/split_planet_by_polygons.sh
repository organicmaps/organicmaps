#!/bin/bash

set -e -u -x

if [[ `hostname` == "Scout" ]]; then
  POLY_FILES_PATH=~/toolchain/v2/data/poly
  PLANET_FILE=~/toolchain/v2/data/planet-latest.o5m
  OUT_DIR=~/toolchain/v2/data/planet-split
  GENERATOR_TOOL=/media/ssd/osm/omim-build-release/out/release/generator_tool
  OSMCONVERT_TOOL=~/toolchain/v2/git/osmctools/osmconvert
else
  POLY_FILES_PATH=~/poly
  PLANET_FILE=~/planet/planet-latest.o5m
  OUT_DIR=~/planet-split
  GENERATOR_TOOL=~/omim-build-release/out/release/generator_tool
  OSMCONVERT_TOOL=~/osmctools/osmconvert
fi

mkdir $POLY_FILES_PATH || true
mkdir $OUT_DIR || true

$GENERATOR_TOOL -export_poly_path $POLY_FILES_PATH

EXT=.poly 
NUM_INSTANCES=8

COUNTRY_LIST=${COUNTRY_LIST-$(ls -1 $POLY_FILES_PATH/*$EXT | xargs -d "\n" basename -s $EXT)}
echo "$COUNTRY_LIST" | xargs -d "\n" -P $NUM_INSTANCES -I % $OSMCONVERT_TOOL $PLANET_FILE --hash-memory=2000 -B=$POLY_FILES_PATH/%.poly --complex-ways --out-pbf -o=$OUT_DIR/%.pbf &>~/split_planet_osmconvert.log
