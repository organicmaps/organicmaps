#!/bin/bash
############################################
# Builds routing indices for given regions #
############################################

set -e # Exit when any of commands fail
set -o pipefail # Capture all errors in command chains
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
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"
BORDERS_PATH="${BORDERS_PATH:-$DATA_PATH/borders}"
TARGET="${TARGET:-$DATA_PATH}"
[ ! -d "$TARGET" ] && fail "$TARGET should be a writable folder"
INTDIR="${INTDIR:-$TARGET/intermediate_data}"
mkdir -p "$INTDIR"
NUM_PROCESSES=${NUM_PROCESSES:-8}
KEEP_INTDIR=${KEEP_INTDIR-}
LOG_PATH=${LOG_PATH:-.}
echo "[$(date +%Y/%m/%d\ %H:%M:%S)] $0 $1"

if [ "$1" == "pbf" ]; then
  PLANET="${PLANET:-$HOME/planet/planet-latest.o5m}"
  OSMCTOOLS="${OSMCTOOLS:-$HOME/osmctools}"
  [ ! -d "$OSMCTOOLS" ] && OSMCTOOLS="$INTDIR"
  [ ! -x "$OSMCTOOLS/osmconvert" ] && cc -x c -O3 "$OMIM_PATH/tools/osmctools/osmconvert.c" -o "$OSMCTOOLS/osmconvert" -lz

  TMPBORDERS="$INTDIR/tmpborders"
  [ -d "$TMPBORDERS" ] && rm -r "$TMPBORDERS"
  mkdir "$TMPBORDERS"
  if [ -z "${REGIONS-}" ]; then
    cp "$BORDERS_PATH"/*.poly "$TMPBORDERS"
  else
    echo "$REGIONS" | xargs -I % cp "%" "$TMPBORDERS"
  fi
  [ -z "$(ls "$TMPBORDERS" | grep '\.poly$')" ] && fail "No regions to create routing files for"
  export OSMCTOOLS
  export PLANET
  export INTDIR
  find "$TMPBORDERS" -maxdepth 1 -name '*.poly' -print0 | xargs -0 -P $NUM_PROCESSES -I % \
    sh -c '"$OSMCTOOLS/osmconvert" "$PLANET" --hash-memory=2000 -B="%" --complex-ways --out-pbf -o="$INTDIR/$(basename "%" .poly).pbf"'
  rm -r "$TMPBORDERS"

elif [ "$1" == "prepare" ]; then
  [ -z "$(ls "$INTDIR" | grep '\.pbf$')" ] && fail "Please build PBF files first"
  OSRM_PATH="${OSRM_PATH:-$OMIM_PATH/3party/osrm/osrm-backend}"
  OSRM_BUILD_PATH="${OSRM_BUILD_PATH:-$OMIM_PATH/../osrm-backend-release}"
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
  [ ! -f "$STXXLCFG" ] && fail "For routing, you need ~/.stxxl file. Run this: echo 'disk=$HOME/stxxl_disk1,400G,syscall' > $STXXLCFG"

  for PBF in "$INTDIR"/*.pbf; do
    OSRM_FILE="${PBF%.*}.osrm"
    RESTRICTIONS_FILE="$OSRM_FILE.restrictions"
    LOG="$LOG_PATH/$(basename "$PBF" .pbf).log"
    rm -f "$OSRM_FILE"
    "$OSRM_BUILD_PATH/osrm-extract" --config "$EXTRACT_CFG" --profile "$PROFILE" "$PBF" >> "$LOG" 2>&1 || true
    "$OSRM_BUILD_PATH/osrm-prepare" --config "$PREPARE_CFG" --profile "$PROFILE" "$OSRM_FILE" -r "$RESTRICTIONS_FILE" >> "$LOG" 2>&1 || true
    "$OSRM_BUILD_PATH/osrm-mapsme" -i "$OSRM_FILE" >> "$LOG" 2>&1 || true
    if [ -s "$OSRM_FILE" ]; then
      [ -z "$KEEP_INTDIR" ] && rm -f "$PBF"
      ONE_OSRM_READY=1
    else
      echo "Failed to create $OSRM_FILE" >> "$LOG"
    fi
  done
  [ -z "${ONE_OSRM_READY-}" ] && fail "No osrm files were prepared"

elif [ "$1" == "mwm" ]; then
  [ -z "$(ls "$INTDIR" | grep '\.osrm$')" ] && fail "Please build OSRM files first"
  source "$(dirname "$0")/find_generator_tool.sh"

  if [ ! -d "$TARGET/borders" -o -z "$(ls "$TARGET/borders" | grep '\.poly$')" ]; then
    # copy polygons to a temporary directory
    POLY_DIR="$TARGET/borders"
    mkdir -p "$POLY_DIR"
    if [ -z "${REGIONS-}" ]; then
      cp "$BORDERS_PATH"/*.poly "$POLY_DIR"
    else
      echo "$REGIONS" | xargs -I % cp "%" "$POLY_DIR"
    fi
  fi

  # Xargs has 255 chars limit for exec string, so we use short variable names.
  export G="$GENERATOR_TOOL"
  export K="--make_routing --make_cross_section"
  export TARGET
  export LOG_PATH
  export DATA_PATH
  set +e
  find "$INTDIR" -maxdepth 1 -name '*.osrm' -print0 | xargs -0 -P $NUM_PROCESSES -I % \
    sh -c 'O="%"; B="$(basename "$O" .osrm)"; "$G" $K --osrm_file_name="$O" --data_path="$TARGET" --user_resource_path="$DATA_PATH" --output="$B" 2>> "$LOG_PATH/$B.log"'
  set -e

  if [ -n "${POLY_DIR-}" ]; then
    # delete temporary polygons
    rm "$POLY_DIR"/*.poly
    if [ -z "$(ls -A "$POLY_DIR")" ]; then
      rmdir "$POLY_DIR"
    fi
  fi

elif [ "$1" == "online" ]; then
  PLANET="${PLANET:-$HOME/planet/planet-latest.o5m}"
  OSMCTOOLS="${OSMCTOOLS:-$HOME/osmctools}"
  [ ! -d "$OSMCTOOLS" ] && OSMCTOOLS="$INTDIR"
  [ ! -x "$OSMCTOOLS/osmconvert" ] && cc -x c -lz -O3 "$OMIM_PATH/tools/osmctools/osmconvert.c" -o "$OSMCTOOLS/osmconvert"

  PBF="$INTDIR/planet.pbf"
  "$OSMCTOOLS/osmconvert" "$PLANET" --hash-memory=2000 --out-pbf -o="$PBF"

  OSRM_PATH="${OSRM_PATH:-$OMIM_PATH/3party/osrm/osrm-backend}"
  OSRM_BUILD_PATH="${OSRM_BUILD_PATH:-$OMIM_PATH/../osrm-backend-release}"
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
  OSRM_FILE="$INTDIR/planet.osrm"
  RESTRICTIONS_FILE="$OSRM_FILE.restrictions"
  LOG="$LOG_PATH/planet.log"
  rm -f "$OSRM_FILE"
  "$OSRM_BUILD_PATH/osrm-extract" --config "$EXTRACT_CFG" --profile "$PROFILE" "$PBF" >> "$LOG" 2>&1
  "$OSRM_BUILD_PATH/osrm-prepare" --config "$PREPARE_CFG" --profile "$PROFILE" "$OSRM_FILE" -r "$RESTRICTIONS_FILE" >> "$LOG" 2>&1
  if [ -s "$OSRM_FILE" ]; then
     [ -z "$KEEP_INTDIR" ] && rm -f "$PBF"
  else
      echo "Failed to create $OSRM_FILE" >> "$LOG"
  fi

else
  fail "Incorrect parameter: $1"
fi
exit 0
