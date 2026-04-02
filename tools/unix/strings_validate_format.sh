#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMAPS_PATH="${OMAPS_PATH:-$(dirname "$0")/../..}"
OMAPS_PATH=$(realpath $OMAPS_PATH)

echo
echo "Looking for unused strings in '$OMAPS_PATH/data/strings/strings.txt'"

for f in data/strings/strings.txt data/strings/types_strings.txt data/strings/sound.txt iphone/plist.txt; do
  echo ">> Validating '$f' ..."
  python3 "$OMAPS_PATH/tools/python/twine/python_twine/twine_cli.py" validate-twine-file $f
done;

