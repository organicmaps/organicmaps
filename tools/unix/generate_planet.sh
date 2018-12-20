#!/bin/bash
###############################################
# Builds the whole planet or selected regions #
###############################################

# Displayed when there are unknown options
usage() {
  echo
  echo "Usage: $0 [-c] [-u] [-l] [-w] [-r] [-d]"
  echo
  echo -e "-u\tUpdate planet until coastline is not broken"
  echo -e "-U\tDownload planet when it is missing"
  echo -e "-l\tGenerate coastlines"
  echo -e "-w\tGenerate a world file"
  echo -e "-r\tGenerate routing files"
  echo -e "-o\tGenerate online routing files"
  echo -e "-d\tGenerate descriptions"
  echo -e "-a\tEquivalent to -ulwrd"
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
  echo -e "OSRM_URL\tURL of the osrm server to build world roads."
  echo -e "SRTM_PATH\tPath to 27k zip files with SRTM data."
  echo -e "OLD_INTDIR\tPath to an intermediate_data directory with older files."
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

warn() {
  echo -n -e "\033[0;31m" >&2
  log 'WARNING:' $@
  echo -n -e "\033[0m" >&2
}

# Print mode start message and store it in the status file
putmode() {
  [ $# -gt 0 ] && log "STATUS" "$@"
  echo "$MFLAGS$MODE" > "$STATUS_FILE"
}

format_version() {
  local planet_version=$1
  local output_format=$2
  if [ "$(uname -s)" == "Darwin" ]; then
    echo $(date -j -u -f "%Y-%m-%dT%H:%M:%SZ" "$planet_version" "+$output_format")
  else
    echo $(date -d "$(echo "$planet_version" | sed -e 's/T/ /')" "+$output_format")
  fi
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
OPT_DESCRIPTIONS=
while getopts ":couUlwrapvhd" opt; do
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
    d)
      OPT_DESCRIPTIONS=1
      ;;
    a)
      OPT_COAST=1
      OPT_WORLD=1
      OPT_UPDATE=1
      OPT_ROUTING=1
      OPT_DESCRIPTIONS=1
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
if [ -n "$OPT_ROUTING" -a -n "${OSRM_URL-}" ]; then
  curl -s "http://$OSRM_URL/way_id" | grep -q position || fail "Cannot access $OSRM_URL"
fi
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
SCRIPTS_PATH="$(dirname "$0")"
if [ -e "$SCRIPTS_PATH/hierarchy_to_countries.py" ]; then
  # In a compact packaging python scripts may be placed along with shell scripts.
  PYTHON_SCRIPTS_PATH="$SCRIPTS_PATH"
else
  PYTHON_SCRIPTS_PATH="$OMIM_PATH/tools/python"
fi
ROADS_SCRIPT="$PYTHON_SCRIPTS_PATH/road_runner.py"
HIERARCHY_SCRIPT="$PYTHON_SCRIPTS_PATH/hierarchy_to_countries.py"
DESCRIPTIONS_DOWNLOADER="$PYTHON_SCRIPTS_PATH/descriptions_downloader.py"
LOCALADS_SCRIPT="$PYTHON_SCRIPTS_PATH/local_ads/mwm_to_csv_4localads.py"
UGC_FILE="${UGC_FILE:-$INTDIR/ugc_db.sqlite3}"
POPULAR_PLACES_FILE="${POPULAR_PLACES_FILE:-$INTDIR/popular_places.csv}"
BOOKING_SCRIPT="$PYTHON_SCRIPTS_PATH/booking_hotels.py"
BOOKING_FILE="${BOOKING_FILE:-$INTDIR/hotels.csv}"
OPENTABLE_SCRIPT="$PYTHON_SCRIPTS_PATH/opentable_restaurants.py"
OPENTABLE_FILE="${OPENTABLE_FILE:-$INTDIR/restaurants.csv}"
VIATOR_SCRIPT="$PYTHON_SCRIPTS_PATH/viator_cities.py"
VIATOR_FILE="${VIATOR_FILE:-$INTDIR/viator.csv}"
CITIES_BOUNDARIES_DATA="${CITIES_BOUNDARIES_DATA:-$INTDIR/cities_boundaries.bin}"
TESTING_SCRIPT="$SCRIPTS_PATH/test_planet.sh"
PYTHON="$(which python2.7)"
PYTHON36="$(which python36)" || PYTHON36="$(which python3.6)"
MWM_VERSION_FORMAT="%s"
COUNTRIES_VERSION_FORMAT="%y%m%d"
LOG_PATH="${LOG_PATH:-$TARGET/logs}"
mkdir -p "$LOG_PATH"
PLANET_LOG="$LOG_PATH/generate_planet.log"
TEST_LOG="$LOG_PATH/test_planet.log"
GENERATE_CAMERA_SECTION="--generate_cameras"
[ -n "${MAIL-}" ] && trap "grep STATUS \"$PLANET_LOG\" | mailx -s \"Generate_planet: build failed at $(hostname)\" \"$MAIL\"; exit 1" SIGTERM ERR
echo -e "\n\n----------------------------------------\n\n" >> "$PLANET_LOG"
log "STATUS" "Start ${DESC-}"

# Run external script to find generator_tool
source "$SCRIPTS_PATH/find_generator_tool.sh"

# Prepare borders
mkdir -p "$TARGET/borders"
if [ -n "$(ls "$TARGET/borders" | grep '\.poly')" ]; then
  # Backup old borders
  BORDERS_BACKUP_PATH="$TARGET/borders.$(date +%Y%m%d%H%M%S)"
  mkdir -p "$BORDERS_BACKUP_PATH"
  log "BORDERS" "Note: old borders from $TARGET/borders were moved to $BORDERS_BACKUP_PATH"
  mv "$TARGET/borders"/*.poly "$BORDERS_BACKUP_PATH"
fi
NO_REGIONS=
if [ -n "${REGIONS:-}" ]; then
  echo "$REGIONS" | xargs -I % cp "%" "$TARGET/borders/"
elif [ -z "${REGIONS-1}" ]; then
  # A user asked specifically for no regions
  NO_REGIONS=1
else
  # Copy borders from $BORDERS_PATH or omim/data/borders
  cp "${BORDERS_PATH:-$DATA_PATH/borders}"/*.poly "$TARGET/borders/"
fi
[ -z "$NO_REGIONS" -a -z "$(ls "$TARGET/borders" | grep '\.poly')" ] && fail "No border polygons found, please use REGIONS or BORDER_PATH variables"
ULIMIT_REQ=$((3 * $(ls "$TARGET/borders" | { grep '\.poly' || true; } | wc -l)))
[ $(ulimit -n) -lt $ULIMIT_REQ ] && fail "Ulimit is too small, you need at least $ULIMIT_REQ (e.g. ulimit -n 4000)"

[ -n "$OPT_CLEAN" -a -d "$INTDIR" ] && rm -r "$INTDIR"
mkdir -p "$INTDIR"
if [ -z "${REGIONS+1}" -a "$(df -m "$INTDIR" | tail -n 1 | awk '{ printf "%d\n", $4 / 1024 }')" -lt "400" ]; then
  warn "You have less than 400 GB for intermediate data, that's not enough for the whole planet."
fi

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

# We need osmconvert both for planet/coasts and for getting the timestamp.
[ ! -x "$OSMCTOOLS/osmconvert" ] && cc -x c -O3 "$OMIM_PATH/tools/osmctools/osmconvert.c" -o "$OSMCTOOLS/osmconvert" -lz

if [ -r "$STATUS_FILE" ]; then
  # Read all control variables from file
  IFS=, read -r OPT_DESCRIPTIONS OPT_ROUTING OPT_UPDATE OPT_COAST OPT_WORLD NO_REGIONS MODE < "$STATUS_FILE"
fi
MFLAGS="$OPT_DESCRIPTIONS,$OPT_ROUTING,$OPT_UPDATE,$OPT_COAST,$OPT_WORLD,$NO_REGIONS,"
if [ -z "${MODE:-}" ]; then
  if [ -n "$OPT_COAST" -o -n "$OPT_UPDATE" ]; then
    MODE=coast
  else
    MODE=inter
  fi
fi

# The building process starts here

# Download booking.com hotels. This takes around 3 hours, just like coastline processing.
if [ ! -f "$BOOKING_FILE" -a -n "${BOOKING_USER-}" -a -n "${BOOKING_PASS-}" ]; then
  log "STATUS" "Step S1: Starting background hotels downloading"
  (
      $PYTHON $BOOKING_SCRIPT --user $BOOKING_USER --password $BOOKING_PASS --path "$INTDIR" --download --translate --output "$BOOKING_FILE" 2>"$LOG_PATH"/booking.log || true
      if [ -f "$BOOKING_FILE" -a "$(wc -l < "$BOOKING_FILE" || echo 0)" -gt 100 ]; then
        echo "Hotels have been downloaded. Please ensure this line is before Step 4." >> "$PLANET_LOG"
      else
        if [ -n "${OLD_INTDIR-}" -a -f "${OLD_INTDIR-}/$(basename "$BOOKING_FILE")" ]; then
          cp "$OLD_INTDIR/$(basename "$BOOKING_FILE")" "$INTDIR"
          warn "Failed to download hotels! Using older hotels list."
        else
          warn "Failed to download hotels!"
        fi
        [ -n "${MAIL-}" ] && tail "$LOG_PATH/booking.log" | mailx -s "Failed to download hotels at $(hostname), please hurry to fix" "$MAIL"
      fi
  ) &
fi

# Download opentable.com restaurants. This takes around 30 minutes.
if [ ! -f "$OPENTABLE_FILE" -a -n "${OPENTABLE_USER-}" -a -n "${OPENTABLE_PASS-}" ]; then
  log "STATUS" "Step S2: Starting background restaurants downloading"
  (
      $PYTHON $OPENTABLE_SCRIPT --client $OPENTABLE_USER --secret $OPENTABLE_PASS --opentable_data "$INTDIR"/opentable.json --download --tsv "$OPENTABLE_FILE" 2>"$LOG_PATH"/opentable.log || true
      if [ -f "$OPENTABLE_FILE" -a "$(wc -l < "$OPENTABLE_FILE" || echo 0)" -gt 100 ]; then
        echo "Restaurants have been downloaded. Please ensure this line is before Step 4." >> "$PLANET_LOG"
      else
        if [ -n "${OLD_INTDIR-}" -a -f "${OLD_INTDIR-}/$(basename "$OPENTABLE_FILE")" ]; then
          cp "$OLD_INTDIR/$(basename "$OPENTABLE_FILE")" "$INTDIR"
          warn "Failed to download restaurants! Using older restaurants list."
        else
          warn "Failed to download restaurants!"
        fi
        [ -n "${MAIL-}" ] && tail "$LOG_PATH/opentable.log" | mailx -s "Failed to download restaurants at $(hostname), please hurry to fix" "$MAIL"
      fi
  ) &
fi

# Download viator.com cities. This takes around 3 seconds.
if [ ! -f "$VIATOR_FILE" -a -n "${VIATOR_KEY-}" ]; then
  log "STATUS" "Step S3: Starting background viator cities downloading"
  (
    $PYTHON $VIATOR_SCRIPT --apikey $VIATOR_KEY --output "$VIATOR_FILE" 2>"$LOG_PATH"/viator.log || true
    if [ -f "$VIATOR_FILE" -a "$(wc -l < "$VIATOR_FILE" || echo 0)" -gt 100 ]; then
      echo "Viator cities have been downloaded. Please ensure this line is before Step 4." >> "$PLANET_LOG"
    else
      if [ -n "${OLD_INTDIR-}" -a -f "${OLD_INTDIR-}/$(basename "$VIATOR_FILE")" ]; then
        cp "$OLD_INTDIR/$(basename "$VIATOR_FILE")" "$INTDIR"
        warn "Failed to download viator cities! Using older viator cities list."
      else
        warn "Failed to download viator cities!"
      fi
      [ -n "${MAIL-}" ] && tail "$LOG_PATH/viator.log" | mailx -s "Failed to download viator cities at $(hostname), please hurry to fix" "$MAIL"
    fi
  ) &
fi

# Download UGC (user generated content) database.
if [ ! -f "$UGC_FILE" -a -n "${UGC_DATABASE_URL-}" ]; then
  putmode "Step UGC: Dowloading UGC database"
  (
    curl "$UGC_DATABASE_URL" --output "$UGC_FILE" --silent || true
    if [ -f "$UGC_FILE" -a "$(wc -l < "$UGC_FILE" || echo 0)" -gt 100 ]; then
      echo "UGC database have been downloaded. Please ensure this line is before Step 4." >> "$PLANET_LOG"
    else
      if [ -n "${OLD_INTDIR-}" -a -f "${OLD_INTDIR-}/$(basename "$UGC_FILE")" ]; then
        cp "$OLD_INTDIR/$(basename "$UGC_FILE")" "$INTDIR"
        warn "Failed to download UGC database! Using older UGC database."
      else
        warn "Failed to download UGC database!"
      fi
      [ -n "${MAIL-}" ] && tail "$LOG_PATH/ugc.log" | mailx -s "Failed to download UGC database at $(hostname), please hurry to fix" "$MAIL"
    fi
  ) &
fi

if [ "$MODE" == "coast" ]; then
  putmode

  [ ! -x "$OSMCTOOLS/osmupdate"  ] && cc -x c     "$OMIM_PATH/tools/osmctools/osmupdate.c"  -o "$OSMCTOOLS/osmupdate"
  [ ! -x "$OSMCTOOLS/osmfilter"  ] && cc -x c -O3 "$OMIM_PATH/tools/osmctools/osmfilter.c"  -o "$OSMCTOOLS/osmfilter"
  if [ -n "$OPT_DOWNLOAD" ]; then
    # Planet download is requested
    log "STATUS" "Step 0: Downloading and converting the planet"
    PLANET_PBF="$(dirname "$PLANET")/planet-latest.osm.pbf"
    curl -s -o "$PLANET_PBF" https://planet.openstreetmap.org/pbf/planet-latest.osm.pbf
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
      [ -n "${OSC-}" ] && OSC_ABS="$(cd "$(dirname "${OSC-}")"; pwd)/$(basename "${OSC-}")"
      (
        cd "$OSMCTOOLS" # osmupdate requires osmconvert in a current directory
        ./osmupdate --drop-author --drop-version --out-o5m -v "$PLANET_ABS" "$PLANET_ABS.new.o5m"
        if [ -f "${OSC_ABS-}" ]; then
          # Backup the planet first
          mv "$PLANET_ABS" "$PLANET_ABS.backup"
          log "WARNING" "Altering the planet file with $OSC, restore it from $PLANET_ABS.backup"
          ./osmconvert "$PLANET_ABS.new.o5m" "$OSC_ABS" -o="$PLANET_ABS.merged.o5m"
          mv -f "$PLANET_ABS.merged.o5m" "$PLANET_ABS.new.o5m"
        fi
      ) >> "$PLANET_LOG" 2>&1
      mv "$PLANET.new.o5m" "$PLANET"
    fi

    if [ -n "$OPT_COAST" ]; then
      log "STATUS" "Step 2: Creating and processing new coastline in $COASTS_O5M"
      # Strip coastlines from the planet to speed up the process
      "$OSMCTOOLS/osmfilter" "$PLANET" --keep= --keep-ways="natural=coastline" --keep-nodes="capital=yes place=town =city" "-o=$COASTS_O5M"
      # Preprocess coastlines to separate intermediate directory
      "$GENERATOR_TOOL" --intermediate_data_path="$INTCOASTSDIR/" --node_storage=map --osm_file_type=o5m --osm_file_name="$COASTS_O5M" \
        -preprocess 2>> "$LOG_PATH/WorldCoasts.log"
      # Generate temporary coastlines file in the coasts intermediate dir
      if ! "$GENERATOR_TOOL" --intermediate_data_path="$INTCOASTSDIR/" --node_storage=map --osm_file_type=o5m --osm_file_name="$COASTS_O5M" \
        --user_resource_path="$DATA_PATH/" -make_coasts -fail_on_coasts 2>&1 | tee -a "$LOG_PATH/WorldCoasts.log" | { grep -i 'not merged\|coastline polygons' || true; }
      then
        log "STATUS" "Coastline merge failed"
        if [ -n "$OPT_UPDATE" ]; then
          [ -n "${MAIL-}" ] && tail -n 50 "$LOG_PATH/WorldCoasts.log" | mailx -s "Coastline merge at $(hostname) failed, next try in $MERGE_INTERVAL minutes" "$MAIL"
          echo "Will try fresh coasts again in $MERGE_INTERVAL minutes, or press a key..."
          read -rs -n 1 -t $(($MERGE_INTERVAL * 60)) || true
          TRY_AGAIN=1
        else
          fail
        fi
      fi
    fi
  done
  # Make a working copy of generated coastlines file
  if [ -n "$OPT_COAST" ]; then
    cp "$INTCOASTSDIR"/WorldCoasts.*geom "$INTDIR"
    cp "$INTCOASTSDIR"/*.csv "$INTDIR" || true
  fi
  [ -z "$KEEP_INTDIR" ] && rm -r "$INTCOASTSDIR"
  # Exit if nothing else was requested
  if [ -n "$OPT_ROUTING" -o -n "$OPT_WORLD" -o -z "$NO_REGIONS" ]; then
    MODE=roads
  else
    log "STATUS" "Nothing but coastline temporary files were requested, finishing"
    MODE=last
  fi
fi

# This mode is started only after updating or processing a planet file
if [ "$MODE" == "roads" ]; then
  if [ -z "${OSRM_URL-}" ]; then
    if [ -n "${OLD_INTDIR-}" -a -f "${OLD_INTDIR-}/ways.csv" ]; then
      cp "$OLD_INTDIR/ways.csv" "$INTDIR"
      warn "OSRM_URL variable is not set. Using older world roads file."
    else
      warn "OSRM_URL variable is not set. World roads will not be calculated."
    fi
  else
    putmode "Step 2a: Generating road networks for the World map"
    $PYTHON "$ROADS_SCRIPT" "$INTDIR" "$OSRM_URL" >>"$LOG_PATH"/road_runner.log
  fi
  MODE=inter
fi

if [ "$MODE" == "inter" ]; then
  putmode "Step 3: Generating intermediate data for all MWMs"
  # 1st pass, run in parallel - preprocess whole planet to speed up generation if all coastlines are correct
  "$GENERATOR_TOOL" --intermediate_data_path="$INTDIR/" --node_storage=$NODE_STORAGE --osm_file_type=o5m --osm_file_name="$PLANET" \
    -preprocess 2>> "$PLANET_LOG"
  MODE=features
fi

# Wait until booking and restaurants lists are downloaded.
wait

if [ "$MODE" == "features" ]; then
  putmode "Step 4: Generating features of everything into $TARGET"
  # Checking for coastlines, can't build proper mwms without them
  if [ ! -s "$INTDIR/WorldCoasts.geom" ]; then
    COASTS="${COASTS-WorldCoasts.geom}"
    if [ ! -s "$COASTS" -a -n "${OLD_INTDIR-}" -a -s "${OLD_INTDIR-}/WorldCoasts.geom" ]; then
      log "Using older coastlines."
      COASTS="$OLD_INTDIR/WorldCoasts.geom"
    fi
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
  [ -n "$OPT_WORLD" -a "$NODE_STORAGE" == "map" ] && warn "generating world files with NODE_STORAGE=map may lead to an out of memory error. Try NODE_STORAGE=mem if it fails."
  [ -f "$BOOKING_FILE" ] && PARAMS_SPLIT="$PARAMS_SPLIT --booking_data=$BOOKING_FILE"
  [ -f "$OPENTABLE_FILE" ] && PARAMS_SPLIT="$PARAMS_SPLIT --opentable_data=$OPENTABLE_FILE"
  [ -f "$VIATOR_FILE" ] && PARAMS_SPLIT="$PARAMS_SPLIT --viator_data=$VIATOR_FILE"
  [ -f "$POPULAR_PLACES_FILE" ] && PARAMS_SPLIT="$PARAMS_SPLIT --popular_places_data=$POPULAR_PLACES_FILE"
  "$GENERATOR_TOOL" --intermediate_data_path="$INTDIR/" \
                    --node_storage=$NODE_STORAGE \
                    --osm_file_type=o5m \
                    --osm_file_name="$PLANET" \
                    --data_path="$TARGET" \
                    --user_resource_path="$DATA_PATH/" \
                    --dump_cities_boundaries \
                    --cities_boundaries_data="$CITIES_BOUNDARIES_DATA" \
                    $PARAMS_SPLIT 2>> "$PLANET_LOG"
  MODE=mwm
fi

# Get version from the planet file
if [ -z "${VERSION-}" ]; then
  PLANET_VERSION="$("$OSMCTOOLS/osmconvert" --out-timestamp "$PLANET")"
  if [[ $PLANET_VERSION == *nvalid* ]]; then
    MWM_VERSION="$(date "+$MWM_VERSION_FORMAT")"
    COUNTRIES_VERSION="$(date "+$COUNTRIES_VERSION_FORMAT")"
  else
    MWM_VERSION="$(format_version "$PLANET_VERSION" "$MWM_VERSION_FORMAT")"
    COUNTRIES_VERSION="$(format_version "$PLANET_VERSION" "$COUNTRIES_VERSION_FORMAT")"
  fi
fi

if [ "$MODE" == "mwm" ]; then
  putmode "Step 5: Building all MWMs of regions and of the whole world into $TARGET"
  # First, check for *.mwm.tmp
  [ -z "$NO_REGIONS" -a -z "$(ls "$INTDIR/tmp" | grep '\.mwm\.tmp')" ] && fail "No .mwm.tmp files found."
  # 3rd pass - do in parallel
  # but separate exceptions for world files to finish them earlier
  PARAMS="--data_path=$TARGET --intermediate_data_path=$INTDIR/ --user_resource_path=$DATA_PATH/ --node_storage=$NODE_STORAGE --planet_version=$MWM_VERSION -generate_geometry -generate_index"
  if [ -n "$OPT_WORLD" ]; then
    (
      "$GENERATOR_TOOL" $PARAMS --output=World 2>> "$LOG_PATH/World.log"
      "$GENERATOR_TOOL" --data_path="$TARGET" \
                        --user_resource_path="$DATA_PATH/" \
                        --generate_search_index \
                        --generate_cities_boundaries \
                        --cities_boundaries_data="$CITIES_BOUNDARIES_DATA" \
                        --output=World 2>> "$LOG_PATH/World.log"
    ) &
    "$GENERATOR_TOOL" $PARAMS --output=WorldCoasts 2>> "$LOG_PATH/WorldCoasts.log" &
  fi

  if [ -z "$NO_REGIONS" ]; then
    PARAMS_WITH_SEARCH="$PARAMS --generate_search_index --cities_boundaries_data=$CITIES_BOUNDARIES_DATA --make_city_roads --generate_maxspeed"
    [ -n "${SRTM_PATH-}" -a -d "${SRTM_PATH-}" ] && PARAMS_WITH_SEARCH="$PARAMS_WITH_SEARCH --srtm_path=$SRTM_PATH"
    [ -f "$UGC_FILE" ] && PARAMS_WITH_SEARCH="$PARAMS_WITH_SEARCH --ugc_data=$UGC_FILE"
    [ -f "$POPULAR_PLACES_FILE" ] && PARAMS_WITH_SEARCH="$PARAMS_WITH_SEARCH --popular_places_data=$POPULAR_PLACES_FILE --generate_popular_places"
    for file in "$INTDIR"/tmp/*.mwm.tmp; do
      if [[ "$file" != *minsk-pass* && "$file" != *World* ]]; then
        BASENAME="$(basename "$file" .mwm.tmp)"
        "$GENERATOR_TOOL" $PARAMS_WITH_SEARCH --output="$BASENAME" 2>> "$LOG_PATH/$BASENAME.log" &
        forky
      fi
    done
    wait
  fi

  if [ -n "$OPT_ROUTING" -a -z "$NO_REGIONS" ]; then
    MODE=routing
  elif [ -n "$OPT_DESCRIPTIONS" -a -z "$NO_REGIONS" ]; then
    MODE=descriptions
  else
    MODE=resources
  fi
fi

if [ "$MODE" == "routing" ]; then
  putmode "Step 6: Using freshly generated *.mwm to create routing files"
  # If *.mwm.osm2ft were moved to INTDIR, let's put them back
  [ -z "$(ls "$TARGET" | grep '\.mwm\.osm2ft')" -a -n "$(ls "$INTDIR" | grep '\.mwm\.osm2ft')" ] && mv "$INTDIR"/*.mwm.osm2ft "$TARGET"

  for file in "$TARGET"/*.mwm; do
    if [[ "$file" != *minsk-pass* && "$file" != *World* ]]; then
      BASENAME="$(basename "$file" .mwm)"
      (
        "$GENERATOR_TOOL" --data_path="$TARGET" --intermediate_data_path="$INTDIR/" --user_resource_path="$DATA_PATH/" \
        --make_cross_mwm --disable_cross_mwm_progress  ${GENERATE_CAMERA_SECTION-} --make_routing_index --generate_traffic_keys --output="$BASENAME" 2>> "$LOG_PATH/$BASENAME.log"
        "$GENERATOR_TOOL" --data_path="$TARGET" --intermediate_data_path="$INTDIR/" --user_resource_path="$DATA_PATH/" \
        --make_transit_cross_mwm --transit_path="$DATA_PATH" --output="$BASENAME" 2>> "$LOG_PATH/$BASENAME.log"
        ) &
      forky
    fi
  done
  wait

  if [ -n "$OPT_DESCRIPTIONS" -a -z "$NO_REGIONS" ]; then
    MODE=descriptions
  else
    MODE=resources
  fi
fi

if [ "$MODE" == "descriptions" ]; then
  putmode "Step 7: Using freshly generated *.mwm to create descriptions files"

  URLS_PATH="$INTDIR/wiki_urls.txt"
  WIKI_PAGES_PATH="$INTDIR/descriptions"
  LOG="$LOG_PATH/descriptions.log"
  LANGS="en ru es"

  "$GENERATOR_TOOL" --intermediate_data_path="$INTDIR/" --user_resource_path="$DATA_PATH/" --dump_wikipedia_urls="$URLS_PATH" 2>> $LOG
  $PYTHON36 $DESCRIPTIONS_DOWNLOADER --i "$URLS_PATH" --o "$WIKI_PAGES_PATH" --langs $LANGS 2>> $LOG

  for file in "$TARGET"/*.mwm; do
    if [[ "$file" != *minsk-pass* && "$file" != *World* ]]; then
      BASENAME="$(basename "$file" .mwm)"
      "$GENERATOR_TOOL" --wikipedia_pages="$WIKI_PAGES_PATH/" --data_path="$TARGET" --user_resource_path="$DATA_PATH/" \
      --output="$BASENAME" 2>> "$LOG_PATH/$BASENAME.log" &
      forky
    fi
  done
  wait
  MODE=resources
fi

# Clean up temporary routing files
[ -n "$(ls "$TARGET" | grep '\.mwm\.osm2ft')" ] && mv "$TARGET"/*.mwm.osm2ft "$INTDIR"

if [ "$MODE" == "resources" ]; then
  putmode "Step 8: Updating resource lists"
  # Update countries list
  $PYTHON $HIERARCHY_SCRIPT --target "$TARGET" --hierarchy "$DATA_PATH/hierarchy.txt" --version "$COUNTRIES_VERSION" \
    --old "$DATA_PATH/old_vs_new.csv" --osm "$DATA_PATH/borders_vs_osm.csv" --output "$TARGET/countries.txt" >> "$PLANET_LOG" 2>&1

  # A quick fix: chmodding to a+rw all generated files
  for file in "$TARGET"/*.mwm*; do
    chmod 0666 "$file"
  done
  chmod 0666 "$TARGET"/countries*.txt

  # Move skipped nodes list from the intermediate directory
  [ -e "$INTDIR/skipped_elements.lst" ] && mv "$INTDIR/skipped_elements.lst" "$TARGET/skipped_elements.lst"

  if [ -n "$OPT_WORLD" ]; then
    # Update external resources
    [ -z "$(ls "$TARGET" | grep '\.ttf')" ] && cp "$DATA_PATH"/*.ttf "$TARGET"
    cp "$DATA_PATH/WorldCoasts_obsolete.mwm" "$TARGET"
    EXT_RES="$TARGET/external_resources.txt"
    echo -n > "$EXT_RES"
    UNAME="$(uname)"
    for file in "$TARGET"/World*.mwm "$TARGET"/*.ttf; do
      if [[ "$file" != *roboto_reg* ]]; then
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

if [ -n "${LOCALADS-}" ]; then
  putmode "Step AD: Generating CSV for local ads database"
  (
    LOCALADS_LOG="$LOG_PATH/localads.log"
    LOCALADS_PATH="$INTDIR/localads"
    mkdir -p "$LOCALADS_PATH"
    $PYTHON "$LOCALADS_SCRIPT" "$TARGET" --osm2ft "$INTDIR" --version "$COUNTRIES_VERSION" --types "$DATA_PATH/types.txt" --output "$LOCALADS_PATH" >> "$LOCALADS_LOG" 2>&1
    LOCALADS_ARCHIVE="localads_$COUNTRIES_VERSION.tgz"
    cd "$LOCALADS_PATH"
    tar -czf "$LOCALADS_ARCHIVE" *.csv >> "$LOCALADS_LOG" 2>&1
    AD_URL="$(curl -s --upload-file "$LOCALADS_ARCHIVE" "https://t.bk.ru/$(basename "$LOCALADS_ARCHIVE")")" >> "$LOCALADS_LOG" 2>&1
    log STATUS "Uploaded localads file: $AD_URL"
  ) &
fi

if [ "$MODE" == "test" -a -z "${SKIP_TESTS-}" ]; then
  putmode "Step 9: Testing data"
  bash "$TESTING_SCRIPT" "$TARGET" "${DELTA_WITH-}" > "$TEST_LOG"
else
  echo "Tests were skipped" > "$TEST_LOG"
fi

wait

# Send both log files via e-mail
if [ -n "${MAIL-}" ]; then
  cat <(grep STATUS "$PLANET_LOG") <(echo ---------------) "$TEST_LOG" | mailx -s "Generate_planet: build completed at $(hostname)" "$MAIL"
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
