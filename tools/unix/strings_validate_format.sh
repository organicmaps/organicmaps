#!/bin/bash
set -e -u -o pipefail

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
OMIM_PATH=$(realpath "$OMIM_PATH")
TWINE_CLI="$OMIM_PATH/tools/python/twine/python_twine/twine_cli.py"

echo
echo "Validating translation string files format"

UI_STRINGS=$(mktemp)
trap 'rm -f "$UI_STRINGS"' EXIT
python3 "$OMIM_PATH/tools/python/merge_ui_strings.py" "$OMIM_PATH/data/strings" "$UI_STRINGS"
# Both UI catalogs require tags. Validate together so references can cross catalogs.
python3 "$TWINE_CLI" validate-twine-file --pedantic "$UI_STRINGS"

for f in data/strings/types_strings.txt data/strings/sound.txt; do
  echo ">> Validating '$f' ..."
  # All definitions in types_strings.txt and sound.txt are intentionally untagged.
  python3 "$TWINE_CLI" validate-twine-file "$OMIM_PATH/$f" | sed '/^WARNING: Definition .* has no tags$/d'
done
