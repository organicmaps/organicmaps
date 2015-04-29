#!/bin/bash

set -u -x

if [ $# -lt 1 ]; then
  echo ''
  echo "Usage: $0 {prepare|mwm}"
  echo ''
  exit 1
fi

fail() {
  [ $# -gt 0 ] && echo "$@" >&2
  exit 1
}

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
BORDERS_PATH="${BORDERS_PATH:-$OMIM_PATH/data/borders}"
TARGET="${TARGET:-$OMIM_PATH/data}"
[ ! -d "$TARGET" ] && fail "$TARGET should be a writable folder"
INTDIR="${INTDIR:-$TARGET/intermediate_data}"
mkdir -p "$INTDIR"
NUM_PROCESSES=${NUM_PROCESSES:-8}
OSRM_FLAG="${OSRM_FLAG:-$INTDIR/osrm_done}"

if [ "$1" == "prepare" ]; then
  rm -f "$OSRM_FLAG"
  PLANET="${PLANET:-$HOME/planet/planet-latest.o5m}"
  OSRM_PATH="${OSRM_PATH:-$OMIM_PATH/3party/osrm/osrm-backend}"
  OSRM_BUILD_PATH="${OSRM_BUILD_PATH:-$OSRM_PATH/build}"
  [ ! -x "$OSRM_BUILD_PATH/osrm-extract" ] && fail "Please compile OSRM binaries to $OSRM_BUILD_PATH"
  OSMCTOOLS="${OSMCTOOLS:-$HOME/osmctools}"
  [ ! -d "$OSMCTOOLS" ] && OSMCTOOLS="$INTDIR"
  [ ! -x "$OSMCTOOLS/osmconvert" ] && wget -q -O - http://m.m.i24.cc/osmconvert.c | cc -x c - -lz -O3 -o "$OSMCTOOLS/osmconvert"

  OSRM_THREADS=${OSRM_THREADS:-15}
  OSRM_MEMORY=${OSRM_MEMORY:-50}
  EXTRACT_CFG="$INTDIR/extractor.ini"
  PREPARE_CFG="$INTDIR/contractor.ini"
  echo "threads = $OSRM_THREADS" > "$EXTRACT_CFG"
  echo "memory = $OSRM_MEMORY" > "$PREPARE_CFG"
  echo "threads = $OSRM_THREADS" >> "$PREPARE_CFG"
  PROFILE="$OSRM_PATH/profiles/car.lua"
  [ $# -gt 1 ] && PROFILE="$2"
  [ ! -r "$PROFILE" ] && fail "Lua profile $PROFILE is not found"

  REGIONS=${REGIONS:-$(ls $BORDERS_PATH/*.poly | xargs -I % basename % .poly)}
  [ -z "$REGIONS" ] && fail "No regions to create routing files for"
  echo "$REGIONS" | xargs -P $NUM_PROCESSES -I % "$OSMCTOOLS/osmconvert" $PLANET --hash-memory=2000 -B=$BORDERS_PATH/%.poly --complex-ways --out-pbf -o=$INTDIR/%.pbf

  export STXXLCFG=~/.stxxl
  echo "$REGIONS" | while read REGION ; do
    OSRM_FILE="$INTDIR/$REGION.osrm"
    rm -f "$OSRM_FILE"
    "$OSRM_BUILD_PATH/osrm-extract" --config "$EXTRACT_CFG" --profile "$PROFILE" "$INTDIR/$REGION.pbf"
    rm -f "$INTDIR/$REGION.pbf"
    "$OSRM_BUILD_PATH/osrm-prepare" --config "$PREPARE_CFG" --profile "$PROFILE" "$OSRM_FILE"
    "$OSRM_BUILD_PATH/osrm-mapsme" -i "$OSRM_FILE"
    [ ! -f "$OSRM_FILE" ] && echo "Failed to create $OSRM_FILE"
  done
  touch "$OSRM_FLAG"

elif [ "$1" == "mwm" ]; then
  [ ! -f "$OSRM_FLAG" ] && fail "Please build OSRM files first"
  source find_generator_tool.sh

  if [ ! -d "$TARGET/borders" -o -z "$(ls "$TARGET/borders" | grep \.poly)" ]; then
    # copy polygons to a temporary directory
    POLY_DIR="$TARGET/borders"
    mkdir -p $POLY_DIR
    cp $BORDERS_PATH/*.poly $POLY_DIR/
  fi

  DATA_PATH="$OMIM_PATH/data/"
  echo "${REGIONS:-$(ls $INTDIR/*.osrm | xargs -I % basename % .osrm)}" | xargs -P $NUM_PROCESSES -I % \
    $GENERATOR_TOOL --make_routing --make_cross_section --osrm_file_name="$INTDIR/%.osrm" --data_path="$TARGET" --user_resource_path="$DATA_PATH" --output="%"

  if [ -n "${POLY_DIR-}" ]; then
    # delete temporary polygons
    rm $POLY_DIR/*.poly
    if [ -z "$(ls -A "$POLY_DIR")" ]; then
      rmdir "$POLY_DIR"
    fi
  fi
else
  fail "Incorrect parameter: $1"
fi
