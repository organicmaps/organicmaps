#!/bin/bash
SCRIPTS_PATH="$(dirname "$0")"
OMIM_PATH="${OMIM_PATH:-$SCRIPTS_PATH/../..}"
"$SCRIPTS_PATH/update_assets_for_version.sh" "$OMIM_PATH/data" "$OMIM_PATH/android/assets"
"$SCRIPTS_PATH/add_assets_mwm-ttf.sh" "$OMIM_PATH/data" "$OMIM_PATH/android/flavors/mwm-ttf-assets"
