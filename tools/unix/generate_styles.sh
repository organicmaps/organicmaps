#!/bin/bash
set -e -u

SCRIPTS_PATH="$(dirname "$0")"
"$SCRIPTS_PATH/generate_symbols.sh"
"$SCRIPTS_PATH/generate_drules.sh"

ANDROID_PATH="$SCRIPTS_PATH/../android"
"$ANDROID_PATH/update_assets.sh"
"$ANDROID_PATH/update_assets_yota.sh"
