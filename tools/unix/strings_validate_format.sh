#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
OMIM_PATH=$(realpath $OMIM_PATH)

echo
echo "Validating translation string files format"

for f in data/strings/strings.txt data/strings/types_strings.txt data/strings/sound.txt iphone/plist.txt; do
  echo ">> Validating '$f' ..."
  python3 "$OMIM_PATH/tools/python/twine/python_twine/twine_cli.py" validate-twine-file $OMIM_PATH/$f
done
