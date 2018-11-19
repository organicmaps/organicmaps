#!/bin/bash
set -e -u -x

OMIM_PATH="$(dirname "$0")/../.."
TWINE="$OMIM_PATH/tools/twine/twine"
STRINGS_PATH="$OMIM_PATH/data/strings"

MERGED_FILE="$(mktemp)"
cat "$STRINGS_PATH"/{strings,partners_strings,types_strings,brands_strings}.txt> "$MERGED_FILE"

# TODO: Add "--untagged --tags android" when tags are properly set.
# TODO: Add validate-strings-file call to check for duplicates (and avoid Android build errors) when tags are properly set.
$TWINE generate-all-localization-files --include translated --format android "$MERGED_FILE" "$OMIM_PATH/android/res/"
$TWINE generate-all-localization-files --format apple "$MERGED_FILE" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
$TWINE generate-all-localization-files --format apple-plural "$MERGED_FILE" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
$TWINE generate-all-localization-files --format apple --file-name InfoPlist.strings "$OMIM_PATH/iphone/plist.txt" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
$TWINE generate-all-localization-files --format jquery "$OMIM_PATH/data/cuisines.txt" "$OMIM_PATH/data/cuisine-strings/"
$TWINE generate-all-localization-files --format jquery "$OMIM_PATH/data/countries_names.txt" "$OMIM_PATH/data/countries-strings/"
$TWINE generate-all-localization-files --format jquery "$OMIM_PATH/data/sound.txt" "$OMIM_PATH/data/sound-strings/"
#$TWINE generate-all-localization-files --include translated --format tizen "$STRINGS_PATH/strings.txt" "$OMIM_PATH/tizen/MapsWithMe/res/" --tags tizen

rm $MERGED_FILE
