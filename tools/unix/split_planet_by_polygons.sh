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

ls $POLY_FILES_PATH | parallel -t -v "$OSMCONVERT_TOOL $PLANET_FILE --hash-memory=2000 -B=$POLY_FILES_PATH/{} --out-o5m -o=$OUT_DIR/{.}.o5m"
