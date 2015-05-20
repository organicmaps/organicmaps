#!/bin/bash
set -e -u -x
MY_PATH=`pwd`
BINARY_PATH="$MY_PATH/../../../build-omim/out/debug/skin_generator"
DATA_PATH="$MY_PATH/../../data"
STYLES_PATH="$DATA_PATH/styles"
PNG_PATH="$DATA_PATH/styles/symbols/png"

rm -r $PNG_PATH || true
ln -s "$STYLES_PATH/yota" $PNG_PATH

"$BINARY_PATH" --symbolWidth 19 --symbolHeight 19 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-yota/basic" --skinSuffix="" \
    --colorCorrection true

rm -r $PNG_PATH || true
ln -s "$STYLES_PATH/ldpi" $PNG_PATH

"$BINARY_PATH" --symbolWidth 16 --symbolHeight 16 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-ldpi/basic" --skinSuffix=""

rm -r $PNG_PATH || true
ln -s "$STYLES_PATH/mdpi" $PNG_PATH

"$BINARY_PATH" --symbolWidth 16 --symbolHeight 16 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-mdpi/basic" --skinSuffix=""

rm -r $PNG_PATH || true
ln -s "$STYLES_PATH/hdpi" $PNG_PATH

"$BINARY_PATH" --symbolWidth 24 --symbolHeight 24 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-hdpi/basic" --skinSuffix=""

rm -r $PNG_PATH || true
ln -s "$STYLES_PATH/xhdpi" $PNG_PATH

"$BINARY_PATH" --symbolWidth 36 --symbolHeight 36 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-xhdpi/basic" --skinSuffix=""

rm -r $PNG_PATH || true
ln -s "$STYLES_PATH/xxhdpi" $PNG_PATH

"$BINARY_PATH" --symbolWidth 48 --symbolHeight 48 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-xxhdpi/basic" --skinSuffix=""

rm -r $PNG_PATH || true
ln -s "$STYLES_PATH/6plus" $PNG_PATH

"$BINARY_PATH" --symbolWidth 38 --symbolHeight 38 \
    --symbolsDir "$DATA_PATH/styles/symbols" \
    --skinName "$DATA_PATH/resources-6plus/basic" --skinSuffix=""

rm -r $PNG_PATH || true
