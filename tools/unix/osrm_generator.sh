#!/bin/bash

set -u -x
# Do not stop on error - some maps doesn't have any highways at all
#set -e

export STXXLCFG=~/.stxxl

# TODO: fix paths for Scout too, now they are for cdn3 only

DATA_PATH=~/planet-split
OSRM_PATH=~/omim/3party/osrm/osrm-backend
PROFILE="$OSRM_PATH/profiles/car.lua"
BIN_PATH="$OSRM_PATH/build"
EXTRACT="$BIN_PATH/osrm-extract"
EXTRACT_CFG="$OSRM_PATH/../extractor.ini"
PREPARE="$BIN_PATH/osrm-prepare"
PREPARE_CFG="$OSRM_PATH/../contractor.ini"
MWM="$BIN_PATH/osrm-mapsme"
LOG=./osrm_`date +%Y_%m_%d__%H_%M`.log

# Writes all arguments with the current timestamp to LOG file. 
log() 
{
   local prefix="[$(date +%Y/%m/%d\ %H:%M:%S)]: "
   echo "${prefix} $@" >> $LOG
}

# Creation of a osrm file. First parameter is a full path to .pbf file.
make_osrm ()
{
  "$EXTRACT" --config "$EXTRACT_CFG" --profile "$PROFILE" "$1"
  local OSRM_FILE=${1%.pbf}.osrm
  "$PREPARE" --config "$PREPARE_CFG" --profile "$PROFILE" "$OSRM_FILE"
  "$MWM" -i "$OSRM_FILE"
}

# Osrm file checker. Writes log message if the .osrm file created.
# First parameter is a full path to .pbf file.
check_osrm ()
{
  local FL=${1%.pbf}.osrm
  if [ -f "$FL" ]; then
     log "[OK] osrm created" "$FL"
  else
     log "[ERROR] osrm creation failed for" "$FL"
  fi
}

# Osrm file cleaner. Remove a .osrm file for given source.
# First parameter is a full path to .pbf file.
clean_osrm ()
{
  local FL=${1%.pbf}.osrm
  rm -f "$FL"
}

# extract and prepare use all available cores
# mwm is very fast
# so we don't parallel it
echo -n > $LOG
log "OSRM files processing started"
# Recreate log file

#pushd "$PROFILE_PATH"

COUNTRY_LIST=${COUNTRY_LIST-$(ls -1 $DATA_PATH/*.pbf)}

echo "$COUNTRY_LIST" | while read file ; do
  clean_osrm "$file"
  make_osrm "$file"
  check_osrm "$file"
done

#popd

log "OSRM generator finished"
