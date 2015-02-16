#!/bin/bash

set -u -x

export STXXLCFG=~/.stxxl

PLANET_FILE="$HOME/planet/planet-latest.o5m"
OSRM_PATH=~/omim/3party/osrm/osrm-backend
PROFILE="$OSRM_PATH/profiles/car.lua"
BIN_PATH="$OSRM_PATH/build"
EXTRACT="$BIN_PATH/osrm-extract"
EXTRACT_CFG="$OSRM_PATH/../extractor.ini"
PREPARE="$BIN_PATH/osrm-prepare"
PREPARE_CFG="$OSRM_PATH/../contractor.ini"

echo Started at `date`

"$EXTRACT" --config "$EXTRACT_CFG" --profile "$PROFILE" "\"$PLANET_FILE\""
"$PREPARE" --config "$PREPARE_CFG" --profile "$PROFILE" "\"${PLANET_FILE/\.*/\.osrm}\""

echo Finished at `date`
