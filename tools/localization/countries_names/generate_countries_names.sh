#!/bin/bash

set -e -u -x

MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

readonly TMP_DIR=$MY_PATH/../../../localization/countries_names-tmp
readonly DATA_DIR=$MY_PATH/../../../localization/countries_names

rm -rf $TMP_DIR
mkdir $TMP_DIR

python $MY_PATH/csv_to_txt.py $DATA_DIR/countries_names.csv $DATA_DIR/countries_names.txt $DATA_DIR/languages.txt

mkdir $TMP_DIR/json

python $MY_PATH/mk_lang_dirs.py $DATA_DIR/languages.txt $TMP_DIR/json/
$MY_PATH/../../twine/twine --format jquery generate-all-string-files $DATA_DIR/countries_names.txt $TMP_DIR/json

readonly OUTPUT_DIR=$MY_PATH/../../../data/countries-strings/
if [ ! -d $OUTPUT_DIR ]; then
  mkdir $OUTPUT_DIR
fi
cp -R $TMP_DIR/json/ $OUTPUT_DIR
rm -rf $TMP_DIR
