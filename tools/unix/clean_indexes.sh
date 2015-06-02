#!/bin/bash

set -u -x
BASE_PATH=$(pwd)
DATA_PATH=$BASE_PATH/../../data

COUNTRY_LIST=${COUNTRY_LIST-$(ls -1 $DATA_PATH/*.mwm)}

if [ "$COUNTRY_LIST" ]
then
  echo "$COUNTRY_LIST" | while read file ; do
    rm -rf "${file%.mwm}/"
  done
fi
