#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMAPS_PATH="${OMAPS_PATH:-$(dirname "$0")/../..}"
OMAPS_PATH=$(realpath $OMAPS_PATH)
echo
echo "Looking for unused strings in '$OMAPS_PATH/data/strings/strings.txt'"

# Run Twine validation
python3 "$OMAPS_PATH/tools/python/twine/python_twine/twine_cli.py" validate-unused-strings \
  $OMAPS_PATH/data/strings/strings.txt \
  --android-src-root $OMAPS_PATH/android \
  --core-src-root $OMAPS_PATH/libs \
  --ios-src-root $OMAPS_PATH/iphone

