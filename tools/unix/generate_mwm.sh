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
  echo
  echo "Usage: $0 \<file.o5m/bz2/pbf\> [\<routing_profile.lua\>]"
  echo
  echo "Useful environment variables:"
  echo
  echo -e "BORDERS_PATH\tPath to *.poly files for cross-mwm routing"
  echo -e "BORDER\tPath and name of a polygon file for the input"
  echo -e "COASTS\tPath and name of WorldCoasts.geom"
  echo -e "TARGET\tWhere to put resulting files"
  echo
  exit 1
fi

fail() {
  [ $# -gt 0 ] && echo "$@" >&2
  exit 1
}

find_osmconvert() {
  # just a guess
  OSMCONVERT="${OSMCONVERT:-$HOME/osmctools/osmconvert}"
  if [ ! -x "$OSMCONVERT" ]; then
    OSMCONVERT="$INTDIR/osmconvert"
    wget -q -O - http://m.m.i24.cc/osmconvert.c | cc -x c - -lz -O3 -o $OSMCONVERT
  fi
}

SOURCE_FILE="$1"
SOURCE_TYPE="${1##*.}"
BASE_NAME="$(basename "$SOURCE_FILE")"
BASE_NAME="${BASE_NAME%%.*}"
TARGET="${TARGET:-$(dirname "$SOURCE_FILE")}"
[ ! -d "$TARGET" ] && fail "$TARGET should be a writable folder"
TBORDERS="$TARGET/borders"
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
COASTS="${COASTS-WorldCoasts.geom}"
if [ -f "$COASTS" ]; then
  if [ ! -f "$TBORDERS/$BASE_NAME.poly" ]; then
    BORDER="${BORDER:-${BORDERS_PATH:-.}/$BASE_NAME.poly}"
    [ ! -f "$BORDER" ] && fail "Please specify polygon file in BORDER for coastline generation"
    [ ! -d "$TBORDERS" ] && CLEAN_BORDERS=1 && mkdir "$TBORDERS"
    CLEAN_POLY=1
    cp "$BORDER" "$TBORDERS/$BASE_NAME.poly"
  fi
  cp "$COASTS" "$INTDIR/WorldCoasts.geom"
  GENERATE_EVERYTHING="$GENERATE_EVERYTHING --emit_coasts=true --split_by_polygons=true"
fi
# Convert everything to o5m
if [ "$SOURCE_TYPE" == "pbf" -o "$SOURCE_TYPE" == "bz2" ]; then
  find_osmconvert
  if [ "$SOURCE_TYPE" == "bz2" ]; then
    bzcat "$SOURCE_FILE" | $OSMCONVERT - --out-o5m "-o=$INTDIR/$BASE_NAME.o5m" || fail
  else
    "$OSMCONVERT" "$SOURCE_FILE" --out-o5m "-o=$INTDIR/$BASE_NAME.o5m"
  fi
  SOURCE_FILE="$INTDIR/$BASE_NAME.o5m"
  SOURCE_TYPE=o5m
fi
if [ "$SOURCE_TYPE" == "o5m" ]; then
  INTDIR_FLAG="$INTDIR_FLAG --osm_file_type=o5m --osm_file_name=$SOURCE_FILE"
  $GENERATOR_TOOL $INTDIR_FLAG --preprocess=true || fail "Preprocessing failed"
  $GENERATOR_TOOL $INTDIR_FLAG --data_path="$TARGET" --user_resource_path="$DATA_PATH" $GENERATE_EVERYTHING --output="$BASE_NAME"
else
  fail "Unsupported source type: $SOURCE_TYPE"
fi

[ -n "${CLEAN_POLY-}" ] && rm "$TBORDERS/$BASE_NAME.poly"
[ -n "${CLEAN_BORDERS-}" ] && rm -r "$TBORDERS"

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
  find_osmconvert
  "$OSMCONVERT" "$SOURCE_FILE" -o=$PBF || fail "Converting to PBF failed"
  "$OSRM_BUILD_PATH/osrm-extract" --config "$EXTRACT_CFG" --profile "$PROFILE" "$PBF" || fail
  rm "$PBF"
  "$OSRM_BUILD_PATH/osrm-prepare" --config "$PREPARE_CFG" --profile "$PROFILE" "$OSRM" -r "$OSRM.restrictions" || fail
  "$OSRM_BUILD_PATH/osrm-mapsme" -i "$OSRM" || fail
  if [ -n "${BORDERS_PATH-}" -a ! -d "$TARGET/borders" ]; then
    [ ! -e "$BORDERS_PATH/$BASE_NAME.poly" ] && fail "You should have a polygon for processed file: $BORDERS_PATH/$BASE_NAME.poly"
    CROSS_MWM="--make_cross_section"
    mkdir "$TBORDERS"
    cp "$BORDERS_PATH"/*.poly "$TBORDERS"
  fi
  $GENERATOR_TOOL --make_routing=true ${CROSS_MWM-} --osrm_file_name="$OSRM" --data_path="$TARGET" --user_resource_path="$DATA_PATH" --output="$BASE_NAME"
  [ -n "${CROSS_MWM-}" ] && rm -r "$TBORDERS"
fi

# This file is needed only for routing generation
rm -f "$TARGET/$BASE_NAME.mwm.osm2ft"
# Remove temporary offsets table
[ -d "$TARGET/$BASE_NAME" ] && rm -r "$TARGET/$BASE_NAME"
