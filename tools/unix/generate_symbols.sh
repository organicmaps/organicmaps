#!/bin/bash
set -e -u -x
MY_PATH=`dirname $0`
BINARY_PATH="$MY_PATH/../../../tools_only-build-release/out/release/skin_generator"
DATA_PATH="$MY_PATH/../../data"

"$BINARY_PATH" --symbolWidth 24 --symbolHeight 24 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --symbolScale 1 --skinName "$DATA_PATH/basic"


"$BINARY_PATH" --symbolWidth 48 --symbolHeight 48 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --symbolScale 2 --skinName "$DATA_PATH/basic_highres"
