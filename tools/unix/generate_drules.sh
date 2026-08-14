#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"
KOTHIC="$OMIM_PATH/tools/kothic/src"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

# Everything is generated into a staging dir and installed into DATA_PATH only after all styles
# have been built. A style that fails validation must not leave the tree with a half-regenerated
# set of files: colors.txt and patterns.txt accumulate across invocations, while visibility.txt
# and classificator.txt are overwritten by every invocation.
STAGE="$TMP_DIR/data"
mkdir -p "$STAGE"

# The only two files libkomwm reads from its -d dir, everything else it touches there is an output.
cp "$DATA_PATH/mapcss-mapping.csv" "$DATA_PATH/mapcss-dynamic.txt" "$STAGE/"

# Keep vehicle last: it produces the visibility.txt & classificator.txt that ship with the apps.
STYLES=(default outdoors cycling vehicle)

# Priority files are re-formatted in place, so stage them too.
for style in "${STYLES[@]}"
do
  mkdir -p "$STAGE/prio/$style"
  cp "$DATA_PATH/styles/$style/include/"priorities_*.prio.txt "$STAGE/prio/$style/"
done

# Build single-variant drules into the temp dir.
for style in "${STYLES[@]}"
do
  for variant in light dark
  do
    echo "Building drawing rules for style $style/$variant"
    python3 "$KOTHIC/libkomwm.py" \
      -s "$DATA_PATH/styles/$style/$variant/style.mapcss" \
      -o "$TMP_DIR/${style}_$variant" \
      -p "$STAGE/prio/$style/" \
      -d "$STAGE"
  done
done

# Pack each family's light + dark variants into a single file with a per-variant color palette.
echo "Packing light/dark variants into family files..."
for style in "${STYLES[@]}"
do
  python3 "$KOTHIC/merge_variants.py" "$STAGE/drules_$style" \
    light "$TMP_DIR/${style}_light.bin" dark "$TMP_DIR/${style}_dark.bin"
done

# The designer builds styles at runtime into a single-variant file; ship the default light one.
cp "$TMP_DIR/default_light.bin" "$STAGE/drules_design.bin"

echo "Exporting transit colors..."
python3 "$OMIM_PATH/tools/python/transit/transit_colors_export.py" \
  "$STAGE/colors.txt" --colors "$DATA_PATH/transit_colors.txt" > /dev/null

# A tools-only file (e.g. for the mwm viewer) that takes the zoom-range union of the light styles.
# Not bundled into the apps.
echo "Merging styles..."
python3 "$KOTHIC/merge_styles.py" \
  "$TMP_DIR/default_light.bin" \
  "$TMP_DIR/vehicle_light.bin" \
  "$TMP_DIR/outdoors_light.bin" \
  "$TMP_DIR/cycling_light.bin" \
  "$STAGE/drules_merged.bin" \
  "$STAGE/drules_merged.txt" \
   > /dev/null

echo "Installing generated files into $DATA_PATH"
for item in classificator.txt types.txt visibility.txt colors.txt patterns.txt \
            drules_design.bin drules_merged.bin drules_merged.txt
do
  mv -f "$STAGE/$item" "$DATA_PATH/$item"
done
for style in "${STYLES[@]}"
do
  mv -f "$STAGE/drules_$style.bin" "$STAGE/drules_$style.txt" "$DATA_PATH/"
  mv -f "$STAGE/prio/$style/"priorities_*.prio.txt "$DATA_PATH/styles/$style/include/"
done
