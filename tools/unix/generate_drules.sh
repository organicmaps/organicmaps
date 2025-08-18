#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"

function BuildDrawingRules() {
  styleType=$1
  styleName=$2
  suffix=${3-}
  echo "Building drawing rules for style $styleType/$styleName"
  # Cleanup
  rm "$DATA_PATH"/drules_proto$suffix.{bin,txt} || true
  # Run script to build style
  python3 "$OMIM_PATH/tools/kothic/src/libkomwm.py" --txt \
    -s "$DATA_PATH/styles/$styleType/$styleName/style.mapcss" \
    -o "$DATA_PATH/drules_proto$suffix" \
    -p "$DATA_PATH/styles/$styleType/include/"
}

# Cleanup
cleanup=(classificator.txt types.txt visibility.txt colors.txt patterns.txt)
for item in ${cleanup[*]}
do
  rm $DATA_PATH/$item || true
done

# Building drawing rules
BuildDrawingRules default  light _default_light
BuildDrawingRules default  dark _default_dark
BuildDrawingRules outdoors  light _outdoors_light
BuildDrawingRules outdoors  dark _outdoors_dark
# Keep vehicle style last to produce same visibility.txt & classificator.txt
BuildDrawingRules vehicle  light _vehicle_light
BuildDrawingRules vehicle  dark _vehicle_dark

# In designer mode we use drules_proto_design file instead of standard ones
cp $OMIM_PATH/data/drules_proto_default_light.bin $OMIM_PATH/data/drules_proto_default_design.bin

echo "Exporting transit colors..."
python3 "$OMIM_PATH/tools/python/transit/transit_colors_export.py" \
  "$DATA_PATH/colors.txt" > /dev/null

echo "Merging styles..."
python3 "$OMIM_PATH/tools/python/stylesheet/drules_merge.py" \
  "$DATA_PATH/drules_proto_default_light.bin" \
  "$DATA_PATH/drules_proto_vehicle_light.bin" \
  "$DATA_PATH/drules_proto_outdoors_light.bin" \
  "$DATA_PATH/drules_proto.bin" \
  "$DATA_PATH/drules_proto.txt" \
   > /dev/null
