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

# extract and prepare use all available cores
# mwm is very fast
# so we don't parallel it

echo Started at `date`

#pushd "$PROFILE_PATH"

for file in "$DATA_PATH"/*.pbf
do
  "$EXTRACT" --config "$EXTRACT_CFG" --profile "$PROFILE" "\"$file\""
  "$PREPARE" --config "$PREPARE_CFG" --profile "$PROFILE" "\"${file/\.*/\.osrm}\""
  "$MWM" -i "${file/\.*/\.osrm}"
done

#popd

echo Finished at `date`
