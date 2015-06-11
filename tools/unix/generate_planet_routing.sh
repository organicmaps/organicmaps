#!/bin/bash
############################################
# Builds routing indices for given regions #
############################################

set -u # Fail on undefined variables
#set -x # Echo every script line

if [ $# -lt 1 ]; then
  echo ''
  echo "Usage:"
  echo "  $0 pbf"
  echo "  $0 prepare"
  echo "  $0 mwm"
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
KEEP_INTDIR=${KEEP_INTDIR-}
OSRM_FLAG="${OSRM_FLAG:-$INTDIR/osrm_done}"
echo "[$(date +%Y/%m/%d\ %H:%M:%S)] $0 $1"

if [ "$1" == "pbf" ]; then
  rm -f "$OSRM_FLAG"
  PLANET="${PLANET:-$HOME/planet/planet-latest.o5m}"
  OSMCTOOLS="${OSMCTOOLS:-$HOME/osmctools}"
  [ ! -d "$OSMCTOOLS" ] && OSMCTOOLS="$INTDIR"
  # The patch increases number of nodes for osmconvert to avoid overflow crash
  [ ! -x "$OSMCTOOLS/osmconvert" ] && wget -q -O - http://m.m.i24.cc/osmconvert.c | sed 's/60004/600004/' | cc -x c - -lz -O3 -o "$OSMCTOOLS/osmconvert"

  TMPBORDERS="$INTDIR/tmpborders"
  mkdir "$TMPBORDERS"
  if [ -z "${REGIONS-}" ]; then
    cp "$BORDERS_PATH"/*.poly "$TMPBORDERS"
  else
    echo "$REGIONS" | xargs -I % cp "$BORDERS_PATH/%.poly" "$TMPBORDERS"
  fi
  [ -z "$(ls "$TMPBORDERS"/*.poly)" ] && fail "No regions to create routing files for"
  export OSMCTOOLS
  export PLANET
  export INTDIR
  find "$TMPBORDERS" -name '*.poly' -print0 | xargs -0 -P $NUM_PROCESSES -I % \
    sh -c '"$OSMCTOOLS/osmconvert" "$PLANET" --hash-memory=2000 -B="%" --complex-ways --out-pbf -o="$INTDIR/$(basename "%" .poly).pbf"'
  [ $? != 0 ] && fail "Failed to process all the regions"
  rm -r "$TMPBORDERS"

elif [ "$1" == "prepare" ]; then
  rm -f "$OSRM_FLAG"
  [ -z "$(ls "$INTDIR"/*.pbf)" ] && fail "Please build PBF files first"
  OSRM_PATH="${OSRM_PATH:-$OMIM_PATH/3party/osrm/osrm-backend}"
  OSRM_BUILD_PATH="${OSRM_BUILD_PATH:-$OSRM_PATH/build}"
  [ ! -x "$OSRM_BUILD_PATH/osrm-extract" ] && fail "Please compile OSRM binaries to $OSRM_BUILD_PATH"

  OSRM_THREADS=${OSRM_THREADS:-15}
  OSRM_MEMORY=${OSRM_MEMORY:-50}
  EXTRACT_CFG="$INTDIR/extractor.ini"
  PREPARE_CFG="$INTDIR/contractor.ini"
  echo "threads = $OSRM_THREADS" > "$EXTRACT_CFG"
  echo "memory = $OSRM_MEMORY" > "$PREPARE_CFG"
  echo "threads = $OSRM_THREADS" >> "$PREPARE_CFG"
  PROFILE="${PROFILE:-$OSRM_PATH/profiles/car.lua}"
  [ $# -gt 1 ] && PROFILE="$2"
  [ ! -r "$PROFILE" ] && fail "Lua profile $PROFILE is not found"

  export STXXLCFG="$HOME/.stxxl"
  for PBF in "$INTDIR"/*.pbf; do
    OSRM_FILE="${PBF%.*}.osrm"
    rm -f "$OSRM_FILE"
    "$OSRM_BUILD_PATH/osrm-extract" --config "$EXTRACT_CFG" --profile "$PROFILE" "$PBF"
    "$OSRM_BUILD_PATH/osrm-prepare" --config "$PREPARE_CFG" --profile "$PROFILE" "$OSRM_FILE"
    "$OSRM_BUILD_PATH/osrm-mapsme" -i "$OSRM_FILE"
    if [ -s "$OSRM_FILE" ]; then
      [ -n "$KEEP_INTDIR" ] && rm -f "$PBF"
      ONE_OSRM_READY=1
    else
      echo "Failed to create $OSRM_FILE"
    fi
  done
  [ -z "${ONE_OSRM_READY-}" ] && fail "No osrm files were prepared"
  touch "$OSRM_FLAG"

elif [ "$1" == "mwm" ]; then
  [ ! -f "$OSRM_FLAG" ] && fail "Please build OSRM files first"
  source find_generator_tool.sh

  if [ ! -d "$TARGET/borders" -o -z "$(ls "$TARGET/borders" | grep \.poly)" ]; then
    # copy polygons to a temporary directory
    POLY_DIR="$TARGET/borders"
    mkdir -p "$POLY_DIR"
    cp "$BORDERS_PATH"/*.poly "$POLY_DIR/"
  fi

  export GENERATOR_TOOL
  export TARGET
  export DATA_PATH="$OMIM_PATH/data/"
  find "$INTDIR" -name '*.osrm' -print0 | xargs -0 -P $NUM_PROCESSES -I % \
    sh -c '"$GENERATOR_TOOL" --make_routing --make_cross_section --osrm_file_name="%" --data_path="$TARGET" --user_resource_path="$DATA_PATH" --output="$(basename "%" .osrm)"'

  if [ -n "${POLY_DIR-}" ]; then
    # delete temporary polygons
    rm "$POLY_DIR"/*.poly
    if [ -z "$(ls -A "$POLY_DIR")" ]; then
      rmdir "$POLY_DIR"
    fi
  fi
else
  fail "Incorrect parameter: $1"
fi
