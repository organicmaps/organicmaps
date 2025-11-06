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
  tag=$4
  filename=${5:-}
  include=translated

  if [ -z "${filename}" ]; then
      filename=''
  else
      filename="--file-name $filename"
  fi

  echo
  echo "Parsing $format strings from prefix '$input_prefix'"

  # Run script to parse string resource
  python3 "$OMIM_PATH/tools/python/twine/python_twine/twine_cli.py" consume-all-localization-files \
    $OMIM_PATH/data/strings/$strings_file \
    $OMIM_PATH/$input_prefix \
    -f $format --tags $tag \
    $filename \
    -a -c -d en

}

ParseStringResource "strings.txt" android/app/src/main/res android android-app
ParseStringResource "strings.txt" android/sdk/src/main/res android android-sdk

ParseStringResource "strings.txt" iphone/Maps/LocalizedStrings apple apple-maps
ParseStringResource "strings.txt" iphone/Maps/LocalizedStrings apple-plural apple-maps
ParseStringResource "strings.txt" iphone/Maps/LocalizedStrings apple apple-infoplist "InfoPlist.strings"
ParseStringResource "strings.txt" iphone/Chart/Chart apple apple-chart
