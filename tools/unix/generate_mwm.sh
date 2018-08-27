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
# - Target path for mwm is the same as o5m path, or supply TARGET
# - Set TMPDIR variable if you have no space in the default temporary location.

set -u
set -e

if [ $# -lt 1 ]; then
  echo
  echo "Usage: $0 \<file.o5m/osm/bz2/pbf\>"
  echo
  echo "Useful environment variables:"
  echo
  echo -e "BORDERS_PATH\tPath to *.poly files for cross-mwm routing"
  echo -e "BORDER\tPath and name of a polygon file for the input"
  echo -e "COASTS\tPath and name of WorldCoasts.geom"
  echo -e "TARGET\tWhere to put resulting files"
  echo -e "OSM2FT\tSet to 1 to keep the osm2ft file"
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
    echo "compiling osmconvert"
    if [ -e "$OMIM_PATH/tools/osmctools/osmconvert.c" ]; then
      cc -x c -O3 "$OMIM_PATH/tools/osmctools/osmconvert.c" -o "$OSMCONVERT" -lz
    else
      curl -s https://raw.githubusercontent.com/mapsme/osmctools/master/osmconvert.c | cc -x c - -O3 -o $OSMCONVERT -lz
    fi
  fi
}

SOURCE_FILE="$1"
SOURCE_TYPE="${1##*.}"
BASE_NAME="$(basename "$SOURCE_FILE")"
BASE_NAME="${BASE_NAME%%.*}"
SCRIPT_PATH="$(dirname "$0")"
TARGET="${TARGET:-$(dirname "$SOURCE_FILE")}"
[ ! -d "$TARGET" ] && fail "$TARGET should be a writable folder"
TBORDERS="$TARGET/borders"
OMIM_PATH="${OMIM_PATH:-$(cd "$SCRIPT_PATH/../.."; pwd)}"
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"
[ ! -r "$DATA_PATH/types.txt" ] && fail "Cannot find classificators in $DATA_PATH, please set correct OMIM_PATH"

GENERATOR_TOOL="${GENERATOR_TOOL-$SCRIPT_PATH/bin/generator_tool}"
[ -x "$SCRIPT_PATH/find_generator_tool.sh" ] && source "$SCRIPT_PATH/find_generator_tool.sh"
[ ! -x "${GENERATOR_TOOL-}" ] && fail "Cannot find generator tool"

if [ "$(uname)" == "Darwin" ]; then
  INTDIR=$(mktemp -d -t mwmgen.XXXXXXXX)
else
  INTDIR=$(mktemp -d)
fi
trap "rm -rf \"${INTDIR}\"" EXIT SIGINT SIGTERM
CITIES_BOUNDARIES_DATA="${CITIES_BOUNDARIES_DATA:-$INTDIR/cities_boundaries.bin}"

# Create MWM file
INTDIR_FLAG="--intermediate_data_path=$INTDIR/ --node_storage=map"
GENERATE_EVERYTHING="--generate_features --generate_geometry --generate_index --generate_search_index --dump_cities_boundaries --cities_boundaries_data=$CITIES_BOUNDARIES_DATA"
[ -n "${HOTELS-}" ] && GENERATE_EVERYTHING="$GENERATE_EVERYTHING --booking_data=$HOTELS"
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
if [ "$SOURCE_TYPE" == "pbf" -o "$SOURCE_TYPE" == "bz2" -o "$SOURCE_TYPE" == "osm" ]; then
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
  if [ -n "${BORDERS_PATH-}" -a -f "${BORDERS_PATH-}/$BASE_NAME.poly" ]; then
    [ ! -d "$TBORDERS" ] && CLEAN_BORDERS=1 && mkdir "$TBORDERS"
    cp "$BORDERS_PATH"/*.poly "$TBORDERS"
    CROSS_MWM="--make_cross_mwm"
  fi

  GENERATE_CAMERA_SECTION="--generate_cameras"

  $GENERATOR_TOOL $INTDIR_FLAG --osm_file_type=o5m --osm_file_name="$SOURCE_FILE" --preprocess=true || fail "Preprocessing failed"
  $GENERATOR_TOOL $INTDIR_FLAG --osm_file_type=o5m --osm_file_name="$SOURCE_FILE" --data_path="$TARGET" --user_resource_path="$DATA_PATH" $GENERATE_EVERYTHING --output="$BASE_NAME"
  $GENERATOR_TOOL $INTDIR_FLAG --data_path="$TARGET" --user_resource_path="$DATA_PATH" --cities_boundaries_data="$CITIES_BOUNDARIES_DATA" --make_city_roads --output="$BASE_NAME"
  $GENERATOR_TOOL $INTDIR_FLAG --data_path="$TARGET" --user_resource_path="$DATA_PATH" ${CROSS_MWM-} ${GENERATE_CAMERA_SECTION-} --make_routing_index --generate_traffic_keys --output="$BASE_NAME"
else
  echo "Unsupported source type: $SOURCE_TYPE" >&2
fi

[ -n "${CLEAN_POLY-}" ] && rm "$TBORDERS/$BASE_NAME.poly"
[ -n "${CLEAN_BORDERS-}" ] && rm -r "$TBORDERS"

# This file is needed only for routing generation
[ -z "${OSM2FT-}" ] && rm -f "$TARGET/$BASE_NAME.mwm.osm2ft"
# Remove temporary offsets table
[ -d "$TARGET/$BASE_NAME" ] && rm -r "$TARGET/$BASE_NAME"

exit 0 # Added so that the exit code of the script is 0, and not whatever the [ -d ] returns.
