#!/bin/bash
set -e -u -o pipefail

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
OMIM_PATH=$(realpath "$OMIM_PATH")
TWINE_CLI="$OMIM_PATH/tools/python/twine/python_twine/twine_cli.py"

echo
echo "Validating translation string files format"

for f in data/strings/strings.txt data/strings/types_strings.txt data/strings/sound.txt; do
  echo ">> Validating '$f' ..."
  if [[ "$f" == "data/strings/strings.txt" ]]; then
    # Generation selects definitions by tag but keeps untagged ones for every target, so a
    # missing 'tags' line silently copies the string into all apps. --pedantic rejects it.
    python3 "$TWINE_CLI" validate-twine-file --pedantic "$OMIM_PATH/$f"
  else
    # All definitions in types_strings.txt and sound.txt are intentionally untagged.
    python3 "$TWINE_CLI" validate-twine-file "$OMIM_PATH/$f" | sed '/^WARNING: Definition .* has no tags$/d'
  fi
done
