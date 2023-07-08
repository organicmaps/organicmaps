#!/bin/bash

set -u -x -e

export STXXLCFG=~/.stxxl

PLANET_FILE="$HOME/planet/planet-latest.o5m"
OSRM_PATH=~/omim/3party/osrm/osrm-backend
PROFILE="$OSRM_PATH/profiles/car.lua"
BIN_PATH=~/osrm-backend-release
EXTRACT="$BIN_PATH/osrm-extract"
EXTRACT_CFG="$OSRM_PATH/../extractor.ini"
PREPARE="$BIN_PATH/osrm-prepare"
PREPARE_CFG="$OSRM_PATH/../contractor.ini"

echo Started at `date`
FILENAME=`basename "$PLANET_FILE"`
DIR=`dirname "$PLANET_FILE"`
"$EXTRACT" --config "$EXTRACT_CFG" --profile "$PROFILE" "$PLANET_FILE"
"$PREPARE" --config "$PREPARE_CFG" --profile "$PROFILE" "$DIR/${FILENAME/\.*/.osrm}"
echo Finished at `date`
