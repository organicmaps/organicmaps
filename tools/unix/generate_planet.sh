#!/bin/bash
###############################################
# Builds the whole planet or selected regions #
###############################################

# Displayed when there are unknown options
usage() {
  echo ''
  echo "Usage: $0 [-c] [-u] [-w] [-r] [-h]"
  echo
  echo -e "-u\tUpdate planet until coastline is not broken"
  echo -e "-U\tDownload planet when it is missing"
  echo -e "-w\tGenerate World and WorldCoasts"
  echo -e "-r\tGenerate routing files"
  echo -e "-c\tClean last pass results if there was an error, and start anew"
  echo -e "-h\tThis help message"
  echo
  echo -e "\tIf there is unfinished job, continues it, ignoring all arguments."
  echo -e "\tUse -c to clean temporary data and build everything from scratch."
  echo
  echo "Useful environment variables:"
  echo
  echo -e "PLANET\tPlanet file to use"
  echo -e "TARGET\tWhere to put resulting files"
  echo -e "REGIONS\tNewline-separated list of border polygons to use. Example usage:"
  echo -e "\tREGIONS=\$(ls ../../data/A*.poly) $0"
  echo -e "NS\tNode storage; use \"map\" when you have less than 64 GB of memory"
  echo
}

# Print an error message and exit
fail() {
  [ $# -gt 0 ] && echo "$@" >&2
  exit 1
}

# Wait until there are no more processes than processors
forky() {
  while [ $(jobs | wc -l) -gt $NUM_PROCESSES ]; do
    sleep 1
  done
}

# Print timestamp and messages to stderr
log() {
   local prefix="[$(date +%Y/%m/%d\ %H:%M:%S)]:"
   echo "${prefix} $@" >&2
   echo "${prefix} $@" >> "$GENERATOR_LOG"
}

# Print mode start message and store it in the status file
putmode() {
  [ $# -gt 0 ] && log "STATUS" "$@"
  echo "$MFLAGS$MODE" > "$STATUS_FILE"
}

# Parse command line parameters
OPT_CLEAN=
OPT_WORLD=
OPT_UPDATE=
OPT_DOWNLOAD=
OPT_ROUTING=
while getopts ":cuwrh" opt; do
  case $opt in
    c)
      OPT_CLEAN=1
      ;;
    w)
      OPT_WORLD=1
      ;;
    u)
      OPT_UPDATE=1
      ;;
    U)
      OPT_DOWNLOAD=1
      OPT_UPDATE=1
      ;;
    r)
      OPT_ROUTING=1
      ;;
    *)
      usage
      fail
      ;;
  esac
done

EXIT_ON_ERROR=${EXIT_ON_ERROR-1}
[ -n "${EXIT_ON_ERROR-}" ] && set -e # Exit when any of commands fail
set -o pipefail # Capture all errors in command chains
set -u # Fail on undefined variables
#set -x # Echo every script line

# Initialize everything. For variables with X="${X:-...}" you can override a default value
PLANET="${PLANET:-$HOME/planet/planet-latest.o5m}"
[ ! -r "$PLANET" -a -z "$OPT_DOWNLOAD" ] && fail "Please put planet file into $PLANET, use -U, or specify correct PLANET variable"
OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
DATA_PATH="$OMIM_PATH/data"
[ ! -r "${DATA_PATH}/types.txt" ] && fail "Cannot find classificators in $DATA_PATH, please set correct OMIM_PATH"
TARGET="${TARGET:-$DATA_PATH}"
mkdir -p "$TARGET"
INTDIR="${INTDIR:-$TARGET/intermediate_data}"
OSMCTOOLS="${OSMCTOOLS:-$HOME/osmctools}"
[ ! -d "$OSMCTOOLS" ] && OSMCTOOLS="$INTDIR"
MERGE_COASTS_DELAY_SEC=2400
# set to "mem" if there is more than 64 GB of memory
NODE_STORAGE=${NODE_STORAGE:-${NS:-mem}}
NUM_PROCESSES=${NUM_PROCESSES:-$(expr $(nproc || echo 8) - 1)}

STATUS_FILE="$INTDIR/status"
OSRM_FLAG="${OSRM_FLAG:-$INTDIR/osrm_done}"
SCRIPTS_PATH="$(dirname $0)"
ROUTING_SCRIPT="$SCRIPTS_PATH/generate_planet_routing.sh"
GENERATOR_LOG="$TARGET/planet_generator.log"
ROUTING_LOG="$TARGET/planet_routing.log"
log "STATUS" "Start"

# Run external script to find generator_tool
source "$SCRIPTS_PATH/find_generator_tool.sh"

# Prepare borders
[ -n "${EXIT_ON_ERROR-}" ] && set +e # Grep returns non-zero status
mkdir -p "$TARGET/borders"
PREV_BORDERS="$(ls "$TARGET/borders" | grep \.poly)"
if [ -n "${REGIONS:-}" ]; then
  # If region files are specified, backup old borders and copy new
  if [ -n "$PREV_BORDERS" ]; then
    BORDERS_BACKUP_PATH="$TARGET/borders.$(date +%Y%m%d%H%M%S)"
    mkdir -p "$BORDERS_BACKUP_PATH"
    log "BORDERS" "Note: old borders from $TARGET/borders were moved to $BORDERS_BACKUP_PATH"
    ( cd "$TARGET/borders"; mv *.poly "$BORDERS_BACKUP_PATH/" )
  fi
  echo "$REGIONS" | xargs -I % cp "%" "$TARGET/borders/"
elif [ -z "$PREV_BORDERS" ]; then
  # If there are no borders, copy them from $BORDERS_PATH
  BORDERS_PATH="${BORDERS_PATH:-$DATA_PATH/borders}"
  cp "$BORDERS_PATH"/*.poly "$TARGET/borders/"
fi
[ -z "$(ls "$TARGET/borders" | grep \.poly)" ] && fail "No border polygons found, please use REGIONS or BORDER_PATH variables"
[ -n "${EXIT_ON_ERROR-}" ] && set -e
ULIMIT_REQ=$(expr 3 \* $(ls "$TARGET/borders" | grep \.poly | wc -l))
[ $(ulimit -n) -lt $ULIMIT_REQ ] && fail "Ulimit is too small, you need at least $ULIMIT_REQ"

# These variables are used by external script(s), namely generate_planet_routing.sh
export GENERATOR_TOOL
export INTDIR
export OMIM_PATH
export OSRM_FLAG
export PLANET
export OSMCTOOLS
export NUM_PROCESSES
export REGIONS= # Routing script might expect something in this variable
export BORDERS_PATH="$TARGET/borders" # Also for the routing script

[ -n "$OPT_CLEAN" -a -d "$INTDIR" ] && rm -r "$INTDIR"
mkdir -p "$INTDIR"

if [ -r "$STATUS_FILE" ]; then
  # Read all control variables from file
  IFS=, read -r OPT_ROUTING OPT_UPDATE OPT_WORLD MODE < "$STATUS_FILE"
fi
MFLAGS="$OPT_ROUTING,$OPT_UPDATE,$OPT_WORLD,"
if [ -z "${MODE:-}" ]; then
  if [ -n "$OPT_WORLD" -o -n "$OPT_UPDATE" ]; then
    MODE=coast
  else
    MODE=inter
  fi
fi

# The building process starts here

if [ "$MODE" == "coast" ]; then
  putmode
  [ ! -x "$OSMCTOOLS/osmconvert" ] && wget -q -O - http://m.m.i24.cc/osmconvert.c | cc -x c - -lz -O3 -o "$OSMCTOOLS/osmconvert"
  [ ! -x "$OSMCTOOLS/osmupdate"  ] && wget -q -O - http://m.m.i24.cc/osmupdate.c  | cc -x c - -o "$OSMCTOOLS/osmupdate"
  [ ! -x "$OSMCTOOLS/osmfilter"  ] && wget -q -O - http://m.m.i24.cc/osmfilter.c  | cc -x c - -O3 -o "$OSMCTOOLS/osmfilter"
  if [ -n "$OPT_DOWNLOAD" ]; then
    # Planet download is requested
    PLANET_PBF="$(dirname "$PLANET")/planet-latest.osm.pbf"
    wget -O "$PLANET_PBF" http://planet.openstreetmap.org/pbf/planet-latest.osm.pbf
    "$OSMCTOOLS/osmconvert" "$PLANET_PBF" --drop-author --drop-version --out-o5m -o=$PLANET
    rm "$PLANET_PBF"
  fi
  [ ! -r "$PLANET" ] && fail "Planet file $PLANET is not found"
  INTCOASTSDIR="$INTDIR/coasts"
  mkdir -p "$INTCOASTSDIR"
  COASTS="$INTCOASTSDIR/coastlines-latest.o5m"
  TRY_AGAIN=1
  while [ -n "$TRY_AGAIN" ]; do
    TRY_AGAIN=
    if [ -n "$OPT_UPDATE" ]; then
      log "STATUS" "Step 1: Updating the planet file $PLANET"
      PLANET_ABS="$(cd "$(dirname "$PLANET")"; pwd)/$(basename "$PLANET")"
      (
        cd "$OSMCTOOLS" # osmupdate requires osmconvert in a current directory
        ./osmupdate --drop-author --drop-version --out-o5m -v "$PLANET_ABS" "$PLANET_ABS.new.o5m"
      )
      mv "$PLANET.new.o5m" "$PLANET"
    fi

    if [ -n "$OPT_WORLD" ]; then
      log "STATUS" "Step 2: Creating and processing new coastline in $COASTS"
      # Strip coastlines from the planet to speed up the process
      "$OSMCTOOLS/osmfilter" "$PLANET" --keep= --keep-ways="natural=coastline" -o=$COASTS
      # Preprocess coastlines to separate intermediate directory
      log "TIMEMARK" "Generate coastlines intermediate"
      [ -n "${EXIT_ON_ERROR-}" ] && set +e # Temporary disable to read error code
      "$GENERATOR_TOOL" --intermediate_data_path="$INTCOASTSDIR/" --node_storage=map --osm_file_type=o5m --osm_file_name="$COASTS" \
        -preprocess 2>> "$GENERATOR_LOG"
      # Generate temporary coastlines file in the coasts intermediate dir
      log "TIMEMARK" "Generate coastlines"
      "$GENERATOR_TOOL" --intermediate_data_path="$INTCOASTSDIR/" --node_storage=map --osm_file_type=o5m --osm_file_name="$COASTS" \
        --user_resource_path="$DATA_PATH/" -make_coasts -fail_on_coasts 2>&1 | tee "$GENERATOR_LOG"
      EXIT_CODE=$?
      [ -n "${EXIT_ON_ERROR-}" ] && set -e

      if [ $EXIT_CODE != 0 ]; then
        log "TIMEMARK" "Coastline merge failed"
        if [ -n "$OPT_UPDATE" ]; then
          date -u
          echo "Will try fresh coasts again in $MERGE_COASTS_DELAY_SEC seconds..."
          sleep $MERGE_COASTS_DELAY_SEC
          TRY_AGAIN=1
        else
          fail
        fi
      fi
    fi
  done
  # make a working copy of generated coastlines file
  [ -n "$OPT_WORLD" ] && cp "$INTCOASTSDIR/WorldCoasts.mwm.tmp" "$INTDIR"
  rm -r "$INTCOASTSDIR"
  MODE=inter
fi

# Starting routing generation as early as we can, since it's done in parallel
if [ -n "$OPT_ROUTING" ]; then
  if [ -e "$OSRM_FLAG" ]; then
    log "start_routing(): OSRM files have been already created, no need to repeat"
  else
    putmode "Starting OSRM files generation"
    ( bash "$ROUTING_SCRIPT" prepare &> "$ROUTING_LOG" )
  fi
fi

if [ "$MODE" == "inter" ]; then
  putmode "Step 3: Generating intermediate data for all MWMs"
  # 1st pass, run in parallel - preprocess whole planet to speed up generation if all coastlines are correct
  "$GENERATOR_TOOL" --intermediate_data_path="$INTDIR/" --node_storage=$NODE_STORAGE --osm_file_type=o5m --osm_file_name="$PLANET" \
    -preprocess 2>> "$GENERATOR_LOG"
  MODE=features
fi

if [ "$MODE" == "features" ]; then
  putmode "Step 4: Generating features of everything into $TARGET"
  # 2nd pass - paralleled in the code
  PARAMS_SPLIT="-split_by_polygons -generate_features"
  [ -n "$OPT_WORLD" ] && PARAMS_SPLIT="$PARAMS_SPLIT -generate_world -emit_coasts"
  [ -n "$OPT_WORLD" -a "$NODE_STORAGE" == "map" ] && log "WARNING: generating world files with NODE_STORAGE=map may lead to an out of memory error. Try NODE_STORAGE=mem if it fails."
  "$GENERATOR_TOOL" --intermediate_data_path="$INTDIR/" --node_storage=$NODE_STORAGE --osm_file_type=o5m --osm_file_name="$PLANET" \
    --data_path="$TARGET" --user_resource_path="$DATA_PATH/" $PARAMS_SPLIT 2>> "$GENERATOR_LOG"
  MODE=mwm
fi

if [ "$MODE" == "mwm" ]; then
  putmode "Step 5: Building all MWMs of regions and of the whole world into $TARGET"
  # 3rd pass - do in parallel
  # but separate exceptions for world files to finish them earlier
  PARAMS="--data_path=$TARGET --user_resource_path=$DATA_PATH/ --node_storage=$NODE_STORAGE -generate_geometry -generate_index"
  log "TIMEMARK" "Generate final mwms"
  if [ -n "$OPT_WORLD" ]; then
    "$GENERATOR_TOOL" $PARAMS --output=World 2>> "$GENERATOR_LOG" &
    "$GENERATOR_TOOL" $PARAMS --output=WorldCoasts 2>> "$GENERATOR_LOG" &
  fi

  PARAMS_WITH_SEARCH="$PARAMS -generate_search_index"
  for file in $TARGET/*.mwm.tmp; do
    if [[ "$file" != *minsk-pass* && "$file" != *World* ]]; then
      filename="$(basename "$file")"
      filename="${filename%.*.*}"
      "$GENERATOR_TOOL" $PARAMS_WITH_SEARCH --output="$filename" 2>> "$GENERATOR_LOG" &
      forky
    fi
  done

  if [ -n "$OPT_WORLD" ]; then
    log "TIMEMARK" "Generate world search index"
    $GENERATOR_TOOL --data_path="$TARGET" --user_resource_path="$DATA_PATH/" -generate_search_index --output=World 2>> "$GENERATOR_LOG"
  fi

  if [ -n "$OPT_ROUTING" ]; then
    MODE=routing
  else
    MODE=resources
  fi
fi

# The following steps require *.mwm and *.osrm files complete
wait

if [ "$MODE" == "routing" ]; then
  putmode "Step 6: Using freshly generated *.mwm and *.osrm to create routing files"
  if [ ! -e "$OSRM_FLAG" ]; then
    log "OSRM files are missing, skipping routing step."
  else
    bash "$ROUTING_SCRIPT" mwm &> "$ROUTING_LOG"
  fi
  MODE=resources
fi

if [ "$MODE" == "resources" ]; then
  putmode "Step 7: Updating resource lists"
  # Update countries list
  [ ! -e "$TARGET/countries.txt" ] && cp "$DATA_PATH/countries.txt" "$TARGET/countries.txt"
  $GENERATOR_TOOL --data_path="$TARGET" --user_resource_path="$DATA_PATH/" -generate_update 2>> "$GENERATOR_LOG"
  # We have no means of finding the resulting file, so let's assume it was magically placed in DATA_PATH
  [ -e "$DATA_PATH/countries.txt.updated" ] && mv "$DATA_PATH/countries.txt.updated" "$TARGET/countries.txt"

  if [ -n "$OPT_WORLD" ]; then
    # Update external resources
    [ -z "$(ls "$TARGET" | grep \.ttf)" ] && cp "$DATA_PATH"/*.ttf "$TARGET"
    EXT_RES="$TARGET/external_resources.txt"
    echo -n > "$EXT_RES"
    for file in "$TARGET"/World*.mwm "$TARGET"/*.ttf; do
      # This line works only on Linux. OSX equivalent: stat -f "%N %z"
      stat -c "%n %s" "$file" | sed 's#^.*/##' >> "$EXT_RES"
    done
  fi
fi

# Cleaning up temporary directories
rm -r "$INTDIR"
log "STATUS" "Done"
