#!/bin/bash
###############################################
# Builds the whole planet or selected regions #
###############################################

# Displayed when there are unknown options
usage() {
  echo
  echo "Usage: $0 [-c] [-u] [-l] [-w] [-r]"
  echo
  echo -e "-u\tUpdate planet until coastline is not broken"
  echo -e "-U\tDownload planet when it is missing"
  echo -e "-l\tGenerate coastlines"
  echo -e "-w\tGenerate a world file"
  echo -e "-r\tGenerate routing files"
  echo -e "-o\tGenerate online routing files"
  echo -e "-a\tEquivalent to -ulwr"
  echo -e "-p\tGenerate only countries, no world and no routing"
  echo -e "-c\tClean last pass results if there was an error, and start anew"
  echo -e "-v\tPrint all commands executed"
  echo -e "-h\tThis help message"
  echo
  echo -e "\tIf there is unfinished job, continues it, ignoring all arguments (use -p if in doubt)."
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
  echo -e "MODE\tA mode to start with: coast, inter, routing, test, etc."
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
OPT_COAST=
OPT_WORLD=
OPT_UPDATE=
OPT_DOWNLOAD=
OPT_ROUTING=
OPT_ONLINE_ROUTING=
while getopts ":couUlwrapvh" opt; do
  case $opt in
    c)
      OPT_CLEAN=1
      ;;
    l)
      OPT_COAST=1
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
      OPT_COAST=1
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

set -e # Exit when any of commands fail
set -o pipefail # Capture all errors in command chains
set -u # Fail on undefined variables

# Initialize everything. For variables with X="${X:-...}" you can override a default value
PLANET="${PLANET:-$HOME/planet}"
[ -d "$PLANET" ] && PLANET="$PLANET/planet-latest.o5m"
[ ! -f "$PLANET" -a -z "$OPT_DOWNLOAD" ] && fail "Please put planet file into $PLANET, use -U, or specify correct PLANET variable"
OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"
[ ! -r "${DATA_PATH}/types.txt" ] && fail "Cannot find classificators in $DATA_PATH, please set correct OMIM_PATH"
[ -n "$OPT_ROUTING" -a ! -f "$HOME/.stxxl" ] && fail "For routing, you need ~/.stxxl file. Run this: echo 'disk=$HOME/stxxl_disk1,400G,syscall' > $HOME/.stxxl"
TARGET="${TARGET:-$DATA_PATH}"
mkdir -p "$TARGET"
INTDIR="${INTDIR:-$TARGET/intermediate_data}"
OSMCTOOLS="${OSMCTOOLS:-$HOME/osmctools}"
[ ! -d "$OSMCTOOLS" ] && OSMCTOOLS="$INTDIR"
MERGE_INTERVAL=${MERGE_INTERVAL:-40}
# set to "mem" if there is more than 64 GB of memory
NODE_STORAGE=${NODE_STORAGE:-${NS:-mem}}
ASYNC_PBF=${ASYNC_PBF-}
KEEP_INTDIR=${KEEP_INTDIR-1}
# nproc is linux-only
if [ "$(uname -s)" == "Darwin" ]; then
  CPUS="$(sysctl -n hw.ncpu)"
else
  CPUS="$(nproc || echo 8)"
fi
NUM_PROCESSES=${NUM_PROCESSES:-$(($CPUS - 1))}

STATUS_FILE="$INTDIR/status"
OSRM_FLAG="$INTDIR/osrm_done"
SCRIPTS_PATH="$(dirname "$0")"
ROUTING_SCRIPT="$SCRIPTS_PATH/generate_planet_routing.sh"
TESTING_SCRIPT="$SCRIPTS_PATH/test_planet.sh"
UPDATE_DATE="$(date +%y%m%d)"
LOG_PATH="${LOG_PATH:-$TARGET/logs}"
mkdir -p "$LOG_PATH"
PLANET_LOG="$LOG_PATH/generate_planet.log"
[ -n "${MAIL-}" ] && trap "grep STATUS \"$PLANET_LOG\" | mailx -s \"Generate_planet: build failed at $(hostname)\" \"$MAIL\"; exit 1" SIGTERM ERR
echo -e "\n\n----------------------------------------\n\n" >> "$PLANET_LOG"
log "STATUS" "Start ${DESC-}"

# Run external script to find generator_tool
source "$SCRIPTS_PATH/find_generator_tool.sh"

# Prepare borders
mkdir -p "$TARGET/borders"
NO_REGIONS=
if [ -n "${REGIONS:-}" ]; then
  # If region files are specified, backup old borders and copy new
  if [ -n "$(ls "$TARGET/borders" | grep '\.poly')" ]; then
    BORDERS_BACKUP_PATH="$TARGET/borders.$(date +%Y%m%d%H%M%S)"
    mkdir -p "$BORDERS_BACKUP_PATH"
    log "BORDERS" "Note: old borders from $TARGET/borders were moved to $BORDERS_BACKUP_PATH"
    mv "$TARGET/borders"/*.poly "$BORDERS_BACKUP_PATH"
  fi
  echo "$REGIONS" | xargs -I % cp "%" "$TARGET/borders/"
elif [ -z "${REGIONS-1}" ]; then
  # A user asked specifically for no regions
  NO_REGIONS=1
elif [ -z "$(ls "$TARGET/borders" | grep '\.poly')" ]; then
  # If there are no borders, copy them from $BORDERS_PATH
  BORDERS_PATH="${BORDERS_PATH:-$DATA_PATH/borders}"
  cp "$BORDERS_PATH"/*.poly "$TARGET/borders/"
fi
[ -z "$NO_REGIONS" -a -z "$(ls "$TARGET/borders" | grep '\.poly')" ] && fail "No border polygons found, please use REGIONS or BORDER_PATH variables"
ULIMIT_REQ=$((3 * $(ls "$TARGET/borders" | { grep '\.poly' || true; } | wc -l)))
[ $(ulimit -n) -lt $ULIMIT_REQ ] && fail "Ulimit is too small, you need at least $ULIMIT_REQ (e.g. ulimit -n 4000)"

# These variables are used by external script(s), namely generate_planet_routing.sh
export GENERATOR_TOOL
export INTDIR
export OMIM_PATH
export PLANET
export OSMCTOOLS
export NUM_PROCESSES
export KEEP_INTDIR
export LOG_PATH
export REGIONS= # Routing script might expect something in this variable
export BORDERS_PATH="$TARGET/borders" # Also for the routing script
export LC_ALL=en_US.UTF-8

[ -n "$OPT_CLEAN" -a -d "$INTDIR" ] && rm -r "$INTDIR"
mkdir -p "$INTDIR"
if [ -z "${REGIONS+1}" -a "$(df -m "$INTDIR" | tail -n 1 | awk '{ printf "%d\n", $4 / 1024 }')" -lt "250" ]; then
  echo "WARNING: You have less than 250 GB for intermediate data, that's not enough for the whole planet."
fi

if [ -r "$STATUS_FILE" ]; then
  # Read all control variables from file
  IFS=, read -r OPT_ROUTING OPT_UPDATE OPT_COAST OPT_WORLD NO_REGIONS MODE < "$STATUS_FILE"
fi
MFLAGS="$OPT_ROUTING,$OPT_UPDATE,$OPT_COAST,$OPT_WORLD,$NO_REGIONS,"
if [ -z "${MODE:-}" ]; then
  if [ -n "$OPT_COAST" -o -n "$OPT_UPDATE" ]; then
    MODE=coast
  else
    MODE=inter
  fi
fi

# The building process starts here

if [ "$MODE" == "coast" ]; then
  putmode
  [ ! -x "$OSMCTOOLS/osmconvert" ] && cc -x c -O3 "$OMIM_PATH/tools/osmctools/osmconvert.c" -o "$OSMCTOOLS/osmconvert" -lz
  [ ! -x "$OSMCTOOLS/osmupdate"  ] && cc -x c     "$OMIM_PATH/tools/osmctools/osmupdate.c"  -o "$OSMCTOOLS/osmupdate"
  [ ! -x "$OSMCTOOLS/osmfilter"  ] && cc -x c -O3 "$OMIM_PATH/tools/osmctools/osmfilter.c"  -o "$OSMCTOOLS/osmfilter"
  if [ -n "$OPT_DOWNLOAD" ]; then
    # Planet download is requested
    log "STATUS" "Step 0: Downloading and converting the planet"
    PLANET_PBF="$(dirname "$PLANET")/planet-latest.osm.pbf"
    curl -s -o "$PLANET_PBF" http://planet.openstreetmap.org/pbf/planet-latest.osm.pbf
    "$OSMCTOOLS/osmconvert" "$PLANET_PBF" --drop-author --drop-version --out-o5m "-o=$PLANET"
    rm "$PLANET_PBF"
  fi
  [ ! -r "$PLANET" ] && fail "Planet file $PLANET is not found"
  INTCOASTSDIR="$INTDIR/coasts"
  mkdir -p "$INTCOASTSDIR"
  COASTS_O5M="$INTCOASTSDIR/coastlines-latest.o5m"
  TRY_AGAIN=1
  while [ -n "$TRY_AGAIN" ]; do
    TRY_AGAIN=
    if [ -n "$OPT_UPDATE" ]; then
      log "STATUS" "Step 1: Updating the planet file $PLANET"
      PLANET_ABS="$(cd "$(dirname "$PLANET")"; pwd)/$(basename "$PLANET")"
      (
        cd "$OSMCTOOLS" # osmupdate requires osmconvert in a current directory
        ./osmupdate --drop-author --drop-version --out-o5m -v "$PLANET_ABS" "$PLANET_ABS.new.o5m"
      ) >> "$PLANET_LOG" 2>&1
      mv "$PLANET.new.o5m" "$PLANET"
    fi

    if [ -n "$OPT_COAST" ]; then
      log "STATUS" "Step 2: Creating and processing new coastline in $COASTS_O5M"
      # Strip coastlines from the planet to speed up the process
      "$OSMCTOOLS/osmfilter" "$PLANET" --keep= --keep-ways="natural=coastline" "-o=$COASTS_O5M"
      # Preprocess coastlines to separate intermediate directory
      "$GENERATOR_TOOL" --intermediate_data_path="$INTCOASTSDIR/" --node_storage=map --osm_file_type=o5m --osm_file_name="$COASTS_O5M" \
        -preprocess 2>> "$LOG_PATH/WorldCoasts.log"
      # Generate temporary coastlines file in the coasts intermediate dir
      if ! "$GENERATOR_TOOL" --intermediate_data_path="$INTCOASTSDIR/" --node_storage=map --osm_file_type=o5m --osm_file_name="$COASTS_O5M" \
        --user_resource_path="$DATA_PATH/" -make_coasts -fail_on_coasts 2>&1 | tee -a "$LOG_PATH/WorldCoasts.log" | { grep -i 'not merged\|coastline polygons' || true; }
      then
        log "STATUS" "Coastline merge failed"
        if [ -n "$OPT_UPDATE" ]; then
          [ -n "${MAIL-}" ] && tail -n 50 "$LOG_PATH/WorldCoasts.log" | mailx -s "Generate_planet: coastline merge failed, next try in $MERGE_INTERVAL minutes" "$MAIL"
          echo "Will try fresh coasts again in $MERGE_INTERVAL minutes, or press a key..."
          read -rs -n 1 -t $(($MERGE_INTERVAL * 60)) || true
          TRY_AGAIN=1
        else
          fail
        fi
      fi
    fi
  done
  # make a working copy of generated coastlines file
  [ -n "$OPT_COAST" ] && cp "$INTCOASTSDIR/WorldCoasts.rawgeom" "$INTDIR"
  [ -n "$OPT_COAST" ] && cp "$INTCOASTSDIR/WorldCoasts.geom" "$INTDIR"
  [ -z "$KEEP_INTDIR" ] && rm -r "$INTCOASTSDIR"
  if [ -n "$OPT_ROUTING" -o -n "$OPT_WORLD" -o -z "$NO_REGIONS" ]; then
    MODE=inter
  else
    log "STATUS" "Nothing but coastline temporary files were requested, finishing"
    MODE=last
  fi
fi

# Starting routing generation as early as we can, since it's done in parallel
if [ -n "$OPT_ROUTING" -a -z "$NO_REGIONS" ]; then
  if [ -e "$OSRM_FLAG" ]; then
    log "start_routing(): OSRM files have been already created, no need to repeat"
  else
    putmode "Step R: Starting OSRM files generation"
    PBF_FLAG="${OSRM_FLAG}_pbf"
    if [ -n "$ASYNC_PBF" -a ! -e "$PBF_FLAG" ]; then
      (
        bash "$ROUTING_SCRIPT" pbf >> "$PLANET_LOG" 2>&1
        touch "$PBF_FLAG"
        bash "$ROUTING_SCRIPT" prepare >> "$PLANET_LOG" 2>&1
        touch "$OSRM_FLAG"
        rm "$PBF_FLAG"
      ) &
    else
      # Osmconvert takes too much memory: it makes sense to not extract pbfs asyncronously
      if [ -e "$PBF_FLAG" ]; then
        log "start_routing(): PBF files have been already created, skipping that step"
      else
        bash "$ROUTING_SCRIPT" pbf >> "$PLANET_LOG" 2>&1
        touch "$PBF_FLAG"
      fi
      (
        bash "$ROUTING_SCRIPT" prepare >> "$PLANET_LOG" 2>&1
        touch "$OSRM_FLAG"
        rm "$PBF_FLAG"
      ) &
    fi
  fi
fi

if [ -n "$OPT_ONLINE_ROUTING" -a -z "$NO_REGIONS" ]; then
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
  if [ ! -s "$INTDIR/WorldCoasts.geom" ]; then
    COASTS="${COASTS-WorldCoasts.geom}"
    if [ -s "$COASTS" ]; then
      cp "$COASTS" "$INTDIR/WorldCoasts.geom"
      RAWGEOM="${COASTS%.*}.rawgeom"
      [ -s "$RAWGEOM" ] && cp "$RAWGEOM" "$INTDIR/WorldCoasts.rawgeom"
    else
      fail "Please prepare coastlines and put WorldCoasts.geom to $INTDIR"
    fi
  fi
  [ -n "$OPT_WORLD" -a ! -s "$INTDIR/WorldCoasts.rawgeom" ] && fail "You need WorldCoasts.rawgeom in $INTDIR to build a world file"
  # 2nd pass - paralleled in the code
  PARAMS_SPLIT="-generate_features -emit_coasts"
  [ -z "$NO_REGIONS" ] && PARAMS_SPLIT="$PARAMS_SPLIT -split_by_polygons"
  [ -n "$OPT_WORLD" ] && PARAMS_SPLIT="$PARAMS_SPLIT -generate_world"
  [ -n "$OPT_WORLD" -a "$NODE_STORAGE" == "map" ] && log "WARNING: generating world files with NODE_STORAGE=map may lead to an out of memory error. Try NODE_STORAGE=mem if it fails."
  "$GENERATOR_TOOL" --intermediate_data_path="$INTDIR/" --node_storage=$NODE_STORAGE --osm_file_type=o5m --osm_file_name="$PLANET" \
    --data_path="$TARGET" --user_resource_path="$DATA_PATH/" $PARAMS_SPLIT 2>> "$PLANET_LOG"
  MODE=mwm
fi

if [ "$MODE" == "mwm" ]; then
  putmode "Step 5: Building all MWMs of regions and of the whole world into $TARGET"
  # First, check for *.mwm.tmp
  [ -z "$NO_REGIONS" -a -z "$(ls "$INTDIR/tmp" | grep '\.mwm\.tmp')" ] && fail "No .mwm.tmp files found."
  # 3rd pass - do in parallel
  # but separate exceptions for world files to finish them earlier
  PARAMS="--data_path=$TARGET --intermediate_data_path=$INTDIR/ --user_resource_path=$DATA_PATH/ --node_storage=$NODE_STORAGE -generate_geometry -generate_index"
  if [ -n "$OPT_WORLD" ]; then
    (
      "$GENERATOR_TOOL" $PARAMS --planet_version="$UPDATE_DATE" --output=World 2>> "$LOG_PATH/World.log"
      "$GENERATOR_TOOL" --data_path="$TARGET" --planet_version="$UPDATE_DATE" --user_resource_path="$DATA_PATH/" -generate_search_index --output=World 2>> "$LOG_PATH/World.log"
    ) &
    "$GENERATOR_TOOL" $PARAMS --planet_version="$UPDATE_DATE" --output=WorldCoasts 2>> "$LOG_PATH/WorldCoasts.log" &
  fi

  if [ -z "$NO_REGIONS" ]; then
    PARAMS_WITH_SEARCH="$PARAMS -generate_search_index"
    for file in "$INTDIR"/tmp/*.mwm.tmp; do
      if [[ "$file" != *minsk-pass* && "$file" != *World* ]]; then
        BASENAME="$(basename "$file" .mwm.tmp)"
        "$GENERATOR_TOOL" $PARAMS_WITH_SEARCH --planet_version="$UPDATE_DATE" --output="$BASENAME" 2>> "$LOG_PATH/$BASENAME.log" &
        forky
      fi
    done
  fi

  if [ -n "$OPT_ROUTING" -a -z "$NO_REGIONS" ]; then
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
    # If *.mwm.osm2ft were moved to INTDIR, let's put them back
    [ -z "$(ls "$TARGET" | grep '\.mwm\.osm2ft')" -a -n "$(ls "$INTDIR" | grep '\.mwm\.osm2ft')" ] && mv "$INTDIR"/*.mwm.osm2ft "$TARGET"
    bash "$ROUTING_SCRIPT" mwm >> "$PLANET_LOG" 2>&1
  fi
  MODE=resources
fi

# Clean up temporary routing files
[ -f "$OSRM_FLAG" ] && rm "$OSRM_FLAG"
[ -n "$(ls "$TARGET" | grep '\.mwm\.osm2ft')" ] && mv "$TARGET"/*.mwm.osm2ft "$INTDIR"

if [ "$MODE" == "resources" ]; then
  putmode "Step 7: Updating resource lists"
  # Update countries list
  [ ! -e "$TARGET/countries.txt" ] && cp "$DATA_PATH/countries.txt" "$TARGET/countries.txt"
  if "$GENERATOR_TOOL" --data_path="$TARGET" --planet_version="$UPDATE_DATE" --user_resource_path="$DATA_PATH/" -generate_update 2>> "$PLANET_LOG"; then
    # We have no means of finding the resulting file, so let's assume it was magically placed in DATA_PATH
    [ -e "$TARGET/countries.txt.updated" ] && mv "$TARGET/countries.txt.updated" "$TARGET/countries.txt"
    # If we know the planet's version, update it in countries.txt
    if [ -n "${UPDATE_DATE-}" ]; then
      # In-place editing works differently on OS X and Linux, hence two steps
      sed -e "s/\"v\":[0-9]\\{6\\}/\"v\":$UPDATE_DATE/" "$TARGET/countries.txt" > "$INTDIR/countries.txt"
      mv "$INTDIR/countries.txt" "$TARGET"
    fi
  fi
  # A quick fix: chmodding to a+rw all generated files
  for file in "$TARGET"/*.mwm*; do
    chmod 0666 "$file"
  done
  chmod 0666 "$TARGET/countries.txt"

  if [ -n "$OPT_WORLD" ]; then
    # Update external resources
    [ -z "$(ls "$TARGET" | grep '\.ttf')" ] && cp "$DATA_PATH"/*.ttf "$TARGET"
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
  fi
  MODE=test
fi

if [ "$MODE" == "test" ]; then
  putmode "Step 8: Testing data"
  TEST_LOG="$LOG_PATH/test_planet.log"
  bash "$TESTING_SCRIPT" "$TARGET" "${DELTA_WITH-}" > "$TEST_LOG"
  # Send both log files via e-mail
  if [ -n "${MAIL-}" ]; then
    cat <(grep STATUS "$PLANET_LOG") <(echo ---------------) "$TEST_LOG" | mailx -s "Generate_planet: build completed at $(hostname)" "$MAIL"
  fi
fi

# Clean temporary indices
if [ -n "$(ls "$TARGET" | grep '\.mwm$')" ]; then
  for mwm in "$TARGET"/*.mwm; do
    BASENAME="${mwm%.mwm}"
    [ -d "$BASENAME" ] && rm -r "$BASENAME"
  done
fi
# Cleaning up temporary directories
rm "$STATUS_FILE"
[ -z "$KEEP_INTDIR" ] && rm -r "$INTDIR"
trap - SIGTERM ERR
log "STATUS" "Done"
