#!/bin/bash

# When text resources for voice notification are updated this script shall be launched
# to regenerate sound_strings.zip. Then sound_strings.zip will be used by the application for TTS systems.

set -e -u -x

MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

readonly SOUND_TMP_DIR=$MY_PATH/../../../sound-tmp

rm -f $MY_PATH/sound_strings.zip
rm -rf $SOUND_TMP_DIR
mkdir $SOUND_TMP_DIR

python $MY_PATH/sound_csv_to_sound_txt.py $MY_PATH/sound.csv $MY_PATH/sound.txt $MY_PATH/languages.txt

mkdir $SOUND_TMP_DIR/json
python $MY_PATH/languages_dir.py $MY_PATH/languages.txt $SOUND_TMP_DIR/json/
$MY_PATH/../../tools/twine/twine --format jquery generate-all-string-files $MY_PATH/sound.txt $SOUND_TMP_DIR/json

cd $SOUND_TMP_DIR/json/
zip -r sound_strings.zip .
mv $SOUND_TMP_DIR/json/sound_strings.zip $MY_PATH/../../data/
cd $MY_PATH
