#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"
KOTHIC="$OMIM_PATH/tools/kothic/src"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

function BuildStyle() {
  styleType=$1
  styleName=$2
  outBase=$3
  echo "Building drawing rules for style $styleType/$styleName"
  # Each invocation also (re)writes the side files (classificator.txt, types.txt, visibility.txt,
  # colors.txt, patterns.txt) into DATA_PATH; the single-variant drules go to a temp dir and are
  # packed into family files below.
  python3 "$KOTHIC/libkomwm.py" \
    -s "$DATA_PATH/styles/$styleType/$styleName/style.mapcss" \
    -o "$outBase" \
    -p "$DATA_PATH/styles/$styleType/include/" \
    -d "$DATA_PATH"
}

# Cleanup the side files produced by libkomwm.
cleanup=(classificator.txt types.txt visibility.txt colors.txt patterns.txt)
for item in ${cleanup[*]}
do
  rm -f "$DATA_PATH/$item"
done

# Build single-variant drules into the temp dir.
BuildStyle default  light    "$TMP_DIR/default_light"
BuildStyle default  dark     "$TMP_DIR/default_dark"
BuildStyle outdoors light    "$TMP_DIR/outdoors_light"
BuildStyle outdoors dark     "$TMP_DIR/outdoors_dark"
# Keep vehicle style last to produce the same visibility.txt & classificator.txt.
BuildStyle vehicle  light    "$TMP_DIR/vehicle_light"
BuildStyle vehicle  dark     "$TMP_DIR/vehicle_dark"

# Pack each family's light + dark variants into a single file with a per-variant color palette.
echo "Packing light/dark variants into family files..."
python3 "$KOTHIC/merge_variants.py" "$DATA_PATH/drules_default" \
  light "$TMP_DIR/default_light.bin"  dark "$TMP_DIR/default_dark.bin"
python3 "$KOTHIC/merge_variants.py" "$DATA_PATH/drules_outdoors" \
  light "$TMP_DIR/outdoors_light.bin" dark "$TMP_DIR/outdoors_dark.bin"
python3 "$KOTHIC/merge_variants.py" "$DATA_PATH/drules_vehicle" \
  light "$TMP_DIR/vehicle_light.bin"  dark "$TMP_DIR/vehicle_dark.bin"

# The designer builds styles at runtime into a single-variant file; ship the default light one.
cp "$TMP_DIR/default_light.bin" "$DATA_PATH/drules_design.bin"

echo "Exporting transit colors..."
python3 "$OMIM_PATH/tools/python/transit/transit_colors_export.py" \
  "$DATA_PATH/colors.txt" > /dev/null

# A tools-only file (e.g. for the mwm viewer) that takes the zoom-range union of the light styles.
# Not bundled into the apps.
echo "Merging styles..."
python3 "$KOTHIC/merge_styles.py" \
  "$TMP_DIR/default_light.bin" \
  "$TMP_DIR/vehicle_light.bin" \
  "$TMP_DIR/outdoors_light.bin" \
  "$DATA_PATH/drules_merged.bin" \
  "$DATA_PATH/drules_merged.txt" \
   > /dev/null
