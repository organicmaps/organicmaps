#!/bin/bash
set -e -u -x
MY_PATH=`dirname $0`
BINARY_PATH="$MY_PATH/../../../omim-build/out/debug/skin_generator"
DATA_PATH="$MY_PATH/../../data"

"$BINARY_PATH" --symbolWidth 22 --symbolHeight 22 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-yota/basic" --skinSuffix="" \
    --colorCorrection true

"$BINARY_PATH" --symbolWidth 16 --symbolHeight 16 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-ldpi/basic" --skinSuffix=""

"$BINARY_PATH" --symbolWidth 16 --symbolHeight 16 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-mdpi/basic" --skinSuffix=""

"$BINARY_PATH" --symbolWidth 24 --symbolHeight 24 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-hdpi/basic" --skinSuffix=""

"$BINARY_PATH" --symbolWidth 36 --symbolHeight 36 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-xhdpi/basic" --skinSuffix=""

"$BINARY_PATH" --symbolWidth 48 --symbolHeight 48 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-xxhdpi/basic" --skinSuffix=""

