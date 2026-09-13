#!/bin/bash
set -e -u -o pipefail

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
OMIM_PATH=$(realpath "$OMIM_PATH")

echo
echo "Validating translation keys in '$OMIM_PATH/data/strings/strings.txt'"

python3 "$OMIM_PATH/tools/python/validate_translation_keys.py"
