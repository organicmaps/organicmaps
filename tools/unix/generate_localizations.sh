#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
OMIM_PATH=$(realpath $OMIM_PATH)
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"


function GenerateStringResource() {
  strings_file=$1
  output_prefix=$2
  format=$3
  tags=$4
  filename=${5:-}
  include=translated

  if [[ $format == apple* ]]
  then
    # Apple strings file should fallback to English (or other language) in case of missing translation.
    include=all
  fi

  if [ -z "${tags}" ]; then
      tags=''
  else
      tags="--tags $tags"
  fi

  if [ -z "${filename}" ]; then
      filename=''
  else
      filename="--file-name $filename"
  fi

  echo
  echo "Building strings for '$format' and prefix '$output_prefix'"

  # Run script to build string resource
  python3 "$OMIM_PATH/tools/python/twine/python_twine/twine_cli.py" generate-all-localization-files \
    $OMIM_PATH/data/strings/$strings_file \
    $OMIM_PATH/$output_prefix \
    -f $format --include $include \
    $filename $tags \
    -r 
}

# Generate Android strings
GenerateStringResource "strings.txt" android/app/src/main/res android android-app
GenerateStringResource "strings.txt" android/sdk/src/main/res android android-sdk

# Generate iPhone strings
GenerateStringResource "strings.txt" iphone/Maps/LocalizedStrings apple apple-maps
GenerateStringResource "strings.txt" iphone/Maps/LocalizedStrings apple-plural apple-maps
GenerateStringResource "strings.txt" iphone/Maps/LocalizedStrings apple apple-infoplist "InfoPlist.strings"
GenerateStringResource "strings.txt" iphone/Chart/Chart apple apple-chart

# Generate Android types strings
GenerateStringResource "types_strings.txt" android/sdk/src/main/res android "" types_strings.xml

# Generate iPhone types strings
GenerateStringResource "types_strings.txt" iphone/Maps/LocalizedStrings apple "" LocalizableTypes.strings
