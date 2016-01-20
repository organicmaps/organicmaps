#!/bin/bash
set -e -u
SCRIPTS_PATH="$(dirname "$0")"
"$SCRIPTS_PATH/generate_symbols.sh"
"$SCRIPTS_PATH/generate_drules.sh"
