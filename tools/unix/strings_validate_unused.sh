#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
OMIM_PATH=$(realpath $OMIM_PATH)

echo
echo "Looking for unused strings in '$OMIM_PATH/data/strings/strings.txt'"

python3 "$OMIM_PATH/tools/python/validate_unused_strings.py" \
  $OMIM_PATH/data/strings/strings.txt \
  --android-src-root $OMIM_PATH/android \
  --core-src-root $OMIM_PATH/libs \
  --ios-src-root $OMIM_PATH/iphone \
  --allowlist $OMIM_PATH/tools/python/strings_unused_allowlist.txt
