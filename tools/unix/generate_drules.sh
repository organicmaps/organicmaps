#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"
STYLES_RAW_PATH="$DATA_PATH/styles-raw"
STYLES_OUT_PATH="$DATA_PATH/styles"

function BuildDrawingRules() {
  styleType=$1
  styleName=$2
  echo "Building drawing rules for style $styleType/$styleName"

  drules_bin="$STYLES_OUT_PATH/$styleType/$styleName/drules.bin"
  drules_txt="$STYLES_RAW_PATH/$styleType/$styleName/drules.txt"

  # Cleanup
  rm $drules_bin $drules_txt || true
  # Run script to build style
  python3 "$OMIM_PATH/tools/kothic/src/libkomwm.py" \
    -s "$STYLES_RAW_PATH/$styleType/$styleName/style.mapcss" \
    -p "$STYLES_RAW_PATH/$styleType/include/" \
    -d "$DATA_PATH/" \
    --output-file "$drules_bin" \
    --output-file-dump "$drules_txt" \
    --output-file-colors "$STYLES_OUT_PATH/default/colors.txt" \
    --output-file-patterns "$STYLES_OUT_PATH/default/patterns.txt"
  # TODO ↑: We do not support separate patterns/colors for different styles yet.
}

# Cleanup
cleanup=(classificator.txt types.txt visibility.txt styles/default/colors.txt styles/default/patterns.txt)
for item in ${cleanup[*]}
do
  rm $DATA_PATH/$item || true
done

# Keep vehicle style last to produce same visibility.txt & classificator.txt
styles_list=(default outdoors vehicle)
style_variants=(light dark)
for style in ${styles_list[*]}
do
  for variant in ${style_variants[*]}
  do
    BuildDrawingRules $style $variant
  done
done

echo "Exporting transit colors..."
python3 "$OMIM_PATH/tools/python/transit/transit_colors_export.py" \
  "$DATA_PATH/styles/default/colors.txt" \
  -c "$DATA_PATH/styles/default/transit_colors.txt" \
  > /dev/null

echo "Merging styles..."
python3 "$OMIM_PATH/tools/python/stylesheet/drules_merge.py" \
  "$STYLES_OUT_PATH/default/light/drules.bin" \
  "$STYLES_OUT_PATH/vehicle/light/drules.bin" \
  "$STYLES_OUT_PATH/outdoors/light/drules.bin" \
  "$STYLES_OUT_PATH/merged/drules_proto.bin" \
  "$STYLES_OUT_PATH/merged/drules_proto.txt" \
   > /dev/null
