#!/bin/bash
###########################
# Builds one specific MWM #
###########################

# Prerequisites:
#
# - The script should be placed in omim/tools/unix, or supply OMIM_PATH with path to omim repo
# - Data path with classificators etc. should be present in $OMIM_PATH/data
#     Inside it should be at least: categories.txt, classificator.txt, types.txt, drules_proto.bin
# - Compiled generator_tool somewhere in omim/../build/out/whatever, or supply BUILD_PATH
# - For routing, compiled OSRM binaries in omim/3party/osrm/osrm-backend/build, or supply OSRM_BUILD_PATH
# - Target path for mwm is the same as o5m path, or supply TARGET

# Cross-borders routing index is not created, since we don't assume
# the source file to be one of the pre-defined countries.

set -u

if [ $# -lt 1 ]; then
  echo ''
  echo "Usage: $0 \<file.o5m/bz2\> [\<routing_profile.lua\>]"
  echo ''
  exit 0
fi

fail() {
  [ $# -gt 0 ] && echo "$@" >&2
  exit 1
}

SOURCE_FILE="$1"
SOURCE_TYPE="${1##*.}"
BASE_NAME="$(basename "$SOURCE_FILE")"
BASE_NAME="${BASE_NAME%%.*}"
TARGET="${TARGET:-$(dirname "$SOURCE_FILE")}"
[ ! -d "$TARGET" ] && fail "$TARGET should be a writable folder"
OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
DATA_PATH="$OMIM_PATH/data/"
[ ! -r "${DATA_PATH}types.txt" ] && fail "Cannot find classificators in $DATA_PATH, please set correct OMIM_PATH"

source find_generator_tool.sh

if [ "$(uname)" == "Darwin" ]; then
  INTDIR=$(mktemp -d -t mwmgen)
else
  INTDIR=$(mktemp -d)
fi
trap "rm -rf \"${INTDIR}\"" EXIT SIGINT SIGTERM

# Create MWM file
INTDIR_FLAG="--intermediate_data_path=$INTDIR/ --node_storage=map"
GENERATE_EVERYTHING='--generate_features=true --generate_geometry=true --generate_index=true --generate_search_index=true'
if [ "$SOURCE_TYPE" == "o5m" ]; then
  INTDIR_FLAG="$INTDIR_FLAG --osm_file_type=o5m --osm_file_name=$SOURCE_FILE"
  $GENERATOR_TOOL $INTDIR_FLAG --preprocess=true || fail "Preprocessing failed"
  $GENERATOR_TOOL $INTDIR_FLAG --data_path="$TARGET" --user_resource_path="$DATA_PATH" $GENERATE_EVERYTHING --output="$BASE_NAME"
elif [ "$SOURCE_TYPE" == "bz2" ]; then
  bzcat "$SOURCE_FILE" | $GENERATOR_TOOL $INTDIR_FLAG --preprocess=true || fail "Preprocessing failed"
  bzcat "$SOURCE_FILE" | $GENERATOR_TOOL $INTDIR_FLAG --data_path="$TARGET" --user_resource_path="$DATA_PATH" $GENERATE_EVERYTHING --output="$BASE_NAME"
else
  fail "Unsupported source type: $SOURCE_TYPE"
fi

if [ $# -gt 1 ]; then
  # Create .mwm.routing file
  OSRM_PATH="${OSRM_PATH:-$OMIM_PATH/3party/osrm/osrm-backend}"
  OSRM_BUILD_PATH="${OSRM_BUILD_PATH:-$OSRM_PATH/build}"
  [ ! -x "$OSRM_BUILD_PATH/osrm-extract" ] && fail "Please compile OSRM binaries to $OSRM_BUILD_PATH"
  [ ! -r "$TARGET/$BASE_NAME.mwm" ] && fail "Please build mwm file beforehand"

  OSRM_THREADS=${OSRM_THREADS:-15}
  OSRM_MEMORY=${OSRM_MEMORY:-50}
  EXTRACT_CFG="$INTDIR/extractor.ini"
  PREPARE_CFG="$INTDIR/contractor.ini"
  echo "threads = $OSRM_THREADS" > "$EXTRACT_CFG"
  echo "memory = $OSRM_MEMORY" > "$PREPARE_CFG"
  echo "threads = $OSRM_THREADS" >> "$PREPARE_CFG"
  if [ -r "$2" ]; then
    PROFILE="$2"
  else
    echo "$2 is not a profile, using standard car.lua" >&2
    PROFILE="$OSRM_PATH/profiles/car.lua"
  fi
  [ ! -r "$PROFILE" ] && fail "Lua profile $PROFILE is not found"

  PBF="$INTDIR/tmp.pbf"
  OSRM="$INTDIR/tmp.osrm"
  export STXXLCFG="$HOME/.stxxl"
  # just a guess
  OSMCONVERT="${OSMCONVERT:-$HOME/osmctools/osmconvert}"
  if [ ! -x "$OSMCONVERT" ]; then
    OSMCONVERT="$INTDIR/osmconvert"
    wget -O - http://m.m.i24.cc/osmconvert.c | cc -x c - -lz -O3 -o $OSMCONVERT
  fi
  if [ "$SOURCE_TYPE" == "bz2" ]; then
    bzcat "$SOURCE_FILE" | $OSMCONVERT - -o=$PBF || fail "Converting to PBF failed"
  else
    "$OSMCONVERT" "$SOURCE_FILE" -o=$PBF || fail "Converting to PBF failed"
  fi
  "$OSRM_BUILD_PATH/osrm-extract" --config "$EXTRACT_CFG" --profile "$PROFILE" "$PBF" || fail
  rm "$PBF"
  "$OSRM_BUILD_PATH/osrm-prepare" --config "$PREPARE_CFG" --profile "$PROFILE" "$OSRM" -r "$OSRM.restrictions" || fail
  "$OSRM_BUILD_PATH/osrm-mapsme" -i "$OSRM" || fail
  if [ -n "${BORDERS_PATH-}" -a ! -d "$TARGET/borders" ]; then
    [ ! -e "$BORDERS_PATH/$BASE_NAME.poly" ] && fail "You should have a polygon for processed file: $BORDERS_PATH/$BASE_NAME.poly"
    CROSS_MWM="--make_cross_section"
    mkdir "$TARGET/borders"
    cp "$BORDERS_PATH"/*.poly "$TARGET/borders/"
  fi
  $GENERATOR_TOOL --make_routing=true ${CROSS_MWM-} --osrm_file_name="$OSRM" --data_path="$TARGET" --user_resource_path="$DATA_PATH" --output="$BASE_NAME"
  [ -n "${CROSS_MWM-}" ] && rm -r "$TARGET/borders"
fi

# This file is needed only for routing generation
rm -f "$TARGET/$BASE_NAME.mwm.osm2ft"
