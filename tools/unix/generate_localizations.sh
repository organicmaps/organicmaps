#!/bin/bash
set -e -u -x

OMIM_PATH="$(dirname "$0")/../.."
TWINE="$OMIM_PATH/tools/twine/twine"

# TODO: Add "--untagged --tags android" when tags are properly set.
# TODO: Add validate-strings-file call to check for duplicates (and avoid Android build errors) when tags are properly set.
$TWINE --format android generate-all-string-files "$OMIM_PATH/strings.txt" "$OMIM_PATH/android/res/"
$TWINE --format apple generate-all-string-files "$OMIM_PATH/strings.txt" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
$TWINE --format apple --file-name InfoPlist.strings generate-all-string-files "$OMIM_PATH/iphone/plist.txt" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
$TWINE --format jquery generate-all-string-files "$OMIM_PATH/data/cuisines.txt" "$OMIM_PATH/data/cuisine-strings/"
$TWINE --format jquery generate-all-string-files "$OMIM_PATH/data/countries_names.txt" "$OMIM_PATH/data/countries-strings/"
#$TWINE --format tizen generate-all-string-files "$OMIM_PATH/strings.txt" "$OMIM_PATH/tizen/MapsWithMe/res/" --tags tizen
