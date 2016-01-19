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
  echo "Building drawing rules for style $styleName"
  # Cleanup
  rm "$DATA_PATH"/drules_proto$suffix.{bin,txt} || true
  # Run script to build style
  python "$OMIM_PATH/tools/kothic/src/libkomwm.py" --txt \
    -s "$DATA_PATH/styles/$styleType/style-$styleName/style.mapcss" \
    -o "$DATA_PATH/drules_proto$suffix"
}

# Cleanup
cleanup=(classificator.txt types.txt visibility.txt colors.txt patterns.txt)
for item in ${cleanup[*]}
do
  rm $DATA_PATH/$item || true
done

# Building drawing rules
BuildDrawingRules clear  clear _clear
BuildDrawingRules clear  night _dark
BuildDrawingRules legacy light _legacy

echo "Merging legacy and new styles"
python "$OMIM_PATH/tools/python/stylesheet/drules_merge.py" \
  "$DATA_PATH/drules_proto_legacy.bin" "$DATA_PATH/drules_proto_clear.bin" \
  "$DATA_PATH/drules_proto.bin" "$DATA_PATH/drules_proto.txt" > /dev/null
