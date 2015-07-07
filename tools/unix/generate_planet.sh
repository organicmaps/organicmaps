#!/bin/bash
###############################################
# Builds the whole planet or selected regions #
###############################################

# Displayed when there are unknown options
usage() {
  echo
  echo "Usage: $0 [-c] [-u] [-w] [-r]"
  echo
  echo -e "-u\tUpdate planet until coastline is not broken"
  echo -e "-U\tDownload planet when it is missing"
  echo -e "-w\tGenerate World and WorldCoasts"
  echo -e "-r\tGenerate routing files"
  echo -e "-o\tGenerate online routing files"
  echo -e "-a\tEquivalent to -uwr"
  echo -e "-c\tClean last pass results if there was an error, and start anew"
  echo -e "-v\tPrint all commands executed"
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
  echo -e "ASYNC_PBF\tGenerate PBF files asynchronously, not in a separate step"
  echo -e "MAIL\tE-mail address to send notifications"
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
   echo "${prefix} $@" >> "$PLANET_LOG"
}

# Print mode start message and store it in the status file
putmode() {
  [ $# -gt 0 ] && log "STATUS" "$@"
  echo "$MFLAGS$MODE" > "$STATUS_FILE"
}

# Do not start processing when there are no arguments
[ $# -eq 0 ] && usage && fail
# Parse command line parameters
OPT_CLEAN=
OPT_WORLD=
OPT_UPDATE=
OPT_DOWNLOAD=
OPT_ROUTING=
OPT_ONLINE_ROUTING=
while getopts ":couUwrapvh" opt; do
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
    o)
      OPT_ONLINE_ROUTING=1
      ;;
    a)
      OPT_WORLD=1
      OPT_UPDATE=1
      OPT_ROUTING=1
      ;;
    p)
      ;;
    v)
      set -x
      ;;
    *)
      usage
      fail
      ;;
  esac
done

EXIT_ON_ERROR=${EXIT_ON_ERROR-1}
[ -n "$EXIT_ON_ERROR" ] && set -e # Exit when any of commands fail
set -o pipefail # Capture all errors in command chains
set -u # Fail on undefined variables

# Initialize everything. For variables with X="${X:-...}" you can override a default value
PLANET="${PLANET:-$HOME/planet/planet-latest.o5m}"
[ ! -r "$PLANET" -a -z "$OPT_DOWNLOAD" ] && fail "Please put planet file into $PLANET, use -U, or specify correct PLANET variable"
OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
DATA_PATH="$OMIM_PATH/data"
[ ! -r "${DATA_PATH}/types.txt" ] && fail "Cannot find classificators in $DATA_PATH, please set correct OMIM_PATH"
[ -n "$OPT_ROUTING" -a ! -f "$HOME/.stxxl" ] && fail "For routing, you need ~/.stxxl file. Run this: echo 'disk=$HOME/stxxl_disk1,400G,syscall' > $HOME/.stxxl"
TARGET="${TARGET:-$DATA_PATH}"
mkdir -p "$TARGET"
INTDIR="${INTDIR:-$TARGET/intermediate_data}"
OSMCTOOLS="${OSMCTOOLS:-$HOME/osmctools}"
[ ! -d "$OSMCTOOLS" ] && OSMCTOOLS="$INTDIR"
MERGE_COASTS_DELAY_SEC=2400
# set to "mem" if there is more than 64 GB of memory
NODE_STORAGE=${NODE_STORAGE:-${NS:-mem}}
ASYNC_PBF=${ASYNC_PBF-}
NUM_PROCESSES=${NUM_PROCESSES:-$(expr $(nproc || echo 8) - 1)}
KEEP_INTDIR=${KEEP_INTDIR-1}

STATUS_FILE="$INTDIR/status"
OSRM_FLAG="${OSRM_FLAG:-$INTDIR/osrm_done}"
SCRIPTS_PATH="$(dirname "$0")"
ROUTING_SCRIPT="$SCRIPTS_PATH/generate_planet_routing.sh"
TESTING_SCRIPT="$SCRIPTS_PATH/test_planet.sh"
LOG_PATH="$TARGET/logs"
mkdir -p "$LOG_PATH"
PLANET_LOG="$LOG_PATH/generate_planet.log"
[ -n "${MAIL-}" ] && trap "grep STATUS \"$PLANET_LOG\" | mailx -s \"Generate_planet: build failed\" \"$MAIL\"; exit 1" SIGINT SIGTERM
log "STATUS" "Start"

# Run external script to find generator_tool
source "$SCRIPTS_PATH/find_generator_tool.sh"

# Prepare borders
[ -n "$EXIT_ON_ERROR" ] && set +e # Grep returns non-zero status
mkdir -p "$TARGET/borders"
PREV_BORDERS="$(ls "$TARGET/borders" | grep \.poly)"
if [ -n "${REGIONS:-}" ]; then
  # If region files are specified, backup old borders and copy new
  if [ -n "$PREV_BORDERS" ]; then
    BORDERS_BACKUP_PATH="$TARGET/borders.$(date +%Y%m%d%H%M%S)"
    mkdir -p "$BORDERS_BACKUP_PATH"
    log "BORDERS" "Note: old borders from $TARGET/borders were moved to $BORDERS_BACKUP_PATH"
    mv "$TARGET/borders"/*.poly "$BORDERS_BACKUP_PATH"
  fi
  echo "$REGIONS" | xargs -I % cp "%" "$TARGET/borders/"
elif [ -z "$PREV_BORDERS" ]; then
  # If there are no borders, copy them from $BORDERS_PATH
  BORDERS_PATH="${BORDERS_PATH:-$DATA_PATH/borders}"
  cp "$BORDERS_PATH"/*.poly "$TARGET/borders/"
fi
[ -z "$(ls "$TARGET/borders" | grep \.poly)" ] && fail "No border polygons found, please use REGIONS or BORDER_PATH variables"
[ -n "$EXIT_ON_ERROR" ] && set -e
ULIMIT_REQ=$(expr 3 \* $(ls "$TARGET/borders" | grep \.poly | wc -l))
[ $(ulimit -n) -lt $ULIMIT_REQ ] && fail "Ulimit is too small, you need at least $ULIMIT_REQ (e.g. ulimit -n 4000)"

# These variables are used by external script(s), namely generate_planet_routing.sh
export GENERATOR_TOOL
export INTDIR
export OMIM_PATH
export OSRM_FLAG
export PLANET
export OSMCTOOLS
export NUM_PROCESSES
export KEEP_INTDIR
export LOG_PATH
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
  # The patch increases number of nodes for osmconvert to avoid overflow crash
  [ ! -x "$OSMCTOOLS/osmconvert" ] && wget -q -O - http://m.m.i24.cc/osmconvert.c | sed 's/60004/600004/' | cc -x c - -lz -O3 -o "$OSMCTOOLS/osmconvert"
  [ ! -x "$OSMCTOOLS/osmupdate"  ] && wget -q -O - http://m.m.i24.cc/osmupdate.c  | cc -x c - -o "$OSMCTOOLS/osmupdate"
  [ ! -x "$OSMCTOOLS/osmfilter"  ] && wget -q -O - http://m.m.i24.cc/osmfilter.c  | cc -x c - -O3 -o "$OSMCTOOLS/osmfilter"
  if [ -n "$OPT_DOWNLOAD" ]; then
    # Planet download is requested
    log "STATUS" "Step 0: Downloading and converting the planet"
    PLANET_PBF="$(dirname "$PLANET")/planet-latest.osm.pbf"
    wget -O "$PLANET_PBF" http://planet.openstreetmap.org/pbf/planet-latest.osm.pbf
    "$OSMCTOOLS/osmconvert" "$PLANET_PBF" --drop-author --drop-version --out-o5m "-o=$PLANET"
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
        ./osmupdate --drop-author --drop-version --out-o5m -v "$PLANET_ABS" "$PLANET_ABS.new.o5m" >> "$PLANET_LOG" 2>&1
      )
      mv "$PLANET.new.o5m" "$PLANET"
    fi

    if [ -n "$OPT_WORLD" ]; then
      log "STATUS" "Step 2: Creating and processing new coastline in $COASTS"
      # Strip coastlines from the planet to speed up the process
      "$OSMCTOOLS/osmfilter" "$PLANET" --keep= --keep-ways="natural=coastline" "-o=$COASTS"
      # Preprocess coastlines to separate intermediate directory
      log "TIMEMARK" "Generate coastlines intermediate"
      [ -n "$EXIT_ON_ERROR" ] && set +e # Temporary disable to read error code
      "$GENERATOR_TOOL" --intermediate_data_path="$INTCOASTSDIR/" --node_storage=map --osm_file_type=o5m --osm_file_name="$COASTS" \
        -preprocess 2>> "$LOG_PATH/WorldCoasts.log"
      # Generate temporary coastlines file in the coasts intermediate dir
      log "TIMEMARK" "Generate coastlines"
      "$GENERATOR_TOOL" --intermediate_data_path="$INTCOASTSDIR/" --node_storage=map --osm_file_type=o5m --osm_file_name="$COASTS" \
        --user_resource_path="$DATA_PATH/" -make_coasts -fail_on_coasts 2>&1 | tee -a "$LOG_PATH/WorldCoasts.log"

      if [ $? != 0 ]; then
        log "TIMEMARK" "Coastline merge failed"
        if [ -n "$OPT_UPDATE" ]; then
          date -u
          echo "Will try fresh coasts again in $MERGE_COASTS_DELAY_SEC seconds, or press a key..."
          read -rs -n 1 -t $MERGE_COASTS_DELAY_SEC
          TRY_AGAIN=1
        else
          fail
        fi
      fi
      [ -n "$EXIT_ON_ERROR" ] && set -e
    fi
  done
  # make a working copy of generated coastlines file
  [ -n "$OPT_WORLD" ] && cp "$INTCOASTSDIR/WorldCoasts.mwm.tmp" "$INTDIR"
  [ -z "$KEEP_INTDIR" ] && rm -r "$INTCOASTSDIR"
  MODE=inter
fi

# Starting routing generation as early as we can, since it's done in parallel
if [ -n "$OPT_ROUTING" ]; then
  if [ -e "$OSRM_FLAG" ]; then
    log "start_routing(): OSRM files have been already created, no need to repeat"
  else
    putmode "Step R: Starting OSRM files generation"
    # If *.mwm.osm2ft were moved to INTDIR, let's put them back
    [ -n "$EXIT_ON_ERROR" ] && set +e # Grep returns non-zero status
    [ -z "$(ls "$TARGET" | grep \.mwm\.osm2ft)" -a -n "$(ls "$INTDIR" | grep \.mwm\.osm2ft)" ] && mv "$INTDIR"/*.mwm.osm2ft "$TARGET"
    [ -n "$EXIT_ON_ERROR" ] && set -e

    if [ -n "$ASYNC_PBF" ]; then
      (
        bash "$ROUTING_SCRIPT" pbf >> "$PLANET_LOG" 2>&1
        bash "$ROUTING_SCRIPT" prepare >> "$PLANET_LOG" 2>&1
      ) &
    else
      # Osmconvert takes too much memory: it makes sense to not extract pbfs asyncronously
      bash "$ROUTING_SCRIPT" pbf >> "$PLANET_LOG" 2>&1
      ( bash "$ROUTING_SCRIPT" prepare >> "$PLANET_LOG" 2>&1 ) &
    fi
  fi
fi

if [ -n "$OPT_ONLINE_ROUTING" ]; then
  putmode "Step RO: Generating OSRM files for osrm-routed server."
  bash "$ROUTING_SCRIPT" online >> "$PLANET_LOG" 2>&1
fi

if [ "$MODE" == "inter" ]; then
  putmode "Step 3: Generating intermediate data for all MWMs"
  # 1st pass, run in parallel - preprocess whole planet to speed up generation if all coastlines are correct
  "$GENERATOR_TOOL" --intermediate_data_path="$INTDIR/" --node_storage=$NODE_STORAGE --osm_file_type=o5m --osm_file_name="$PLANET" \
    -preprocess 2>> "$PLANET_LOG"
  MODE=features
fi

if [ "$MODE" == "features" ]; then
  putmode "Step 4: Generating features of everything into $TARGET"
  # Checking for coastlines, can't build proper mwms without them
  [ ! -s "$INTDIR/WorldCoasts.mwm.tmp" ] && fail "Please prepare coastlines and put WorldCoasts.mwm.tmp to $INTDIR"
  # 2nd pass - paralleled in the code
  PARAMS_SPLIT="-split_by_polygons -generate_features -emit_coasts"
  [ -n "$OPT_WORLD" ] && PARAMS_SPLIT="$PARAMS_SPLIT -generate_world"
  [ -n "$OPT_WORLD" -a "$NODE_STORAGE" == "map" ] && log "WARNING: generating world files with NODE_STORAGE=map may lead to an out of memory error. Try NODE_STORAGE=mem if it fails."
  "$GENERATOR_TOOL" --intermediate_data_path="$INTDIR/" --node_storage=$NODE_STORAGE --osm_file_type=o5m --osm_file_name="$PLANET" \
    --data_path="$TARGET" --user_resource_path="$DATA_PATH/" $PARAMS_SPLIT 2>> "$PLANET_LOG"
  MODE=mwm
fi

if [ "$MODE" == "mwm" ]; then
  putmode "Step 5: Building all MWMs of regions and of the whole world into $TARGET"
  # 3rd pass - do in parallel
  # but separate exceptions for world files to finish them earlier
  PARAMS="--data_path=$TARGET --user_resource_path=$DATA_PATH/ --node_storage=$NODE_STORAGE -generate_geometry -generate_index"
  if [ -n "$OPT_WORLD" ]; then
    (
      "$GENERATOR_TOOL" $PARAMS --output=World 2>> "$LOG_PATH/World.log"
      "$GENERATOR_TOOL" --data_path="$TARGET" --user_resource_path="$DATA_PATH/" -generate_search_index --output=World 2>> "$LOG_PATH/World.log"
    ) &
    "$GENERATOR_TOOL" $PARAMS --output=WorldCoasts 2>> "$LOG_PATH/WorldCoasts.log" &
  fi

  PARAMS_WITH_SEARCH="$PARAMS -generate_search_index"
  for file in "$TARGET"/*.mwm.tmp; do
    if [[ "$file" != *minsk-pass* && "$file" != *World* ]]; then
      BASENAME="$(basename "$file" .mwm.tmp)"
      "$GENERATOR_TOOL" $PARAMS_WITH_SEARCH --output="$BASENAME" 2>> "$LOG_PATH/$BASENAME.log" &
      forky
    fi
  done

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
    bash "$ROUTING_SCRIPT" mwm >> "$PLANET_LOG" 2>&1
  fi
  MODE=resources
fi

if [ "$MODE" == "resources" ]; then
  putmode "Step 7: Updating resource lists"
  # Update countries list
  [ ! -e "$TARGET/countries.txt" ] && cp "$DATA_PATH/countries.txt" "$TARGET/countries.txt"
  "$GENERATOR_TOOL" --data_path="$TARGET" --user_resource_path="$DATA_PATH/" -generate_update 2>> "$PLANET_LOG"
  # We have no means of finding the resulting file, so let's assume it was magically placed in DATA_PATH
  [ -e "$TARGET/countries.txt.updated" ] && mv "$TARGET/countries.txt.updated" "$TARGET/countries.txt"
  # A quick fix: chmodding to a+rw all generated files
  for file in "$TARGET"/*.mwm*; do
    chmod 0666 "$file"
  done
  chmod 0666 "$TARGET/countries.txt"

  if [ -n "$OPT_WORLD" ]; then
    # Update external resources
    [ -n "$EXIT_ON_ERROR" ] && set +e # Grep returns non-zero status
    [ -z "$(ls "$TARGET" | grep \.ttf)" ] && cp "$DATA_PATH"/*.ttf "$TARGET"
    EXT_RES="$TARGET/external_resources.txt"
    echo -n > "$EXT_RES"
    UNAME="$(uname)"
    for file in "$TARGET"/World*.mwm "$TARGET"/*.ttf; do
      if [[ "$file" != *roboto* ]]; then
        if [ "$UNAME" == "Darwin" ]; then
          stat -f "%N %z" "$file" | sed 's#^.*/##' >> "$EXT_RES"
        else
          stat -c "%n %s" "$file" | sed 's#^.*/##' >> "$EXT_RES"
        fi
      fi
    done
    chmod 0666 "$EXT_RES"
    [ -n "$EXIT_ON_ERROR" ] && set -e
  fi
  MODE=test
fi

if [ "$MODE" == "test" ]; then
  putmode "Step 8: Testing data"
  TEST_LOG="$LOG_PATH/test_planet.log"
  bash "$TESTING_SCRIPT" "$TARGET" > "$TEST_LOG"
  # Send both log files via e-mail
  if [ -n "${MAIL-}" ]; then
    cat <(grep STATUS "$PLANET_LOG") <(echo ---------------) "$TEST_LOG" | mailx -s "Generate_planet: build completed" "$MAIL"
  fi
fi

# Cleaning up temporary directories
rm "$STATUS_FILE" "$OSRM_FLAG"
mv "$TARGET"/*.mwm.osm2ft "$INTDIR"
[ -z "$KEEP_INTDIR" ] && rm -r "$INTDIR"
log "STATUS" "Done"
