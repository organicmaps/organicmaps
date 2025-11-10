#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
OMIM_PATH=$(realpath $OMIM_PATH)
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"


function ParseStringResource() {
  strings_file=$1
  input_prefix=$2
  format=$3
  tags=$4
  filename=${5:-}
  args=${@:6}
  include=translated

  if [ -z "${filename}" ]; then
      filename=''
  else
      filename="--file-name $filename"
  fi

  if [ -z "${tags}" ]; then
      tags=''
  else
      tags="--tags $tags"
  fi

  echo
  echo "Parsing $format strings from prefix '$input_prefix'"

  # Run script to parse string resource
  python3 "$OMIM_PATH/tools/python/twine/python_twine/twine_cli.py" consume-all-localization-files \
    $OMIM_PATH/data/strings/$strings_file \
    $OMIM_PATH/$input_prefix \
    -f $format \
    $tags \
    $filename \
    -a -c -d en $args

}

# Parse Android strings
ParseStringResource "strings.txt" android/app/src/main/res android android-app
ParseStringResource "strings.txt" android/sdk/src/main/res android android-sdk

# Parse iPhone strings
ParseStringResource "strings.txt" iphone/Maps/LocalizedStrings apple apple-maps
ParseStringResource "strings.txt" iphone/Maps/LocalizedStrings apple-plural apple-maps
ParseStringResource "strings.txt" iphone/Maps/LocalizedStrings apple apple-infoplist "InfoPlist.strings"
ParseStringResource "strings.txt" iphone/Chart/Chart apple apple-chart

# Parse Android types strings
ParseStringResource "types_strings.txt" android/sdk/src/main/res android "" "types_strings.xml"

# Parse iPhone types strings
ParseStringResource "types_strings.txt" iphone/Maps/LocalizedStrings apple "" "LocalizableTypes.strings"

ParseStringResource "sound.txt" data/sound-strings jquery "" "" "--fallback-to-default"
