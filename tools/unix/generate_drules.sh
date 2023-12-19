#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"

VALIDATE_ONLY=0

while getopts ":v" option
do
  case "${option}"
  in
    v) VALIDATE_ONLY=1;;
    ?) echo "Usage: ./generate_drules.sh [-v]"; echo "(v) Validates mapcss files only, does not generate."; exit 1;;
  esac
done

if [[ $VALIDATE_ONLY -eq 1 ]]
then
  set +e # Nonzero response codes are not a problem in this section
  EXITCODE=0

  # Find compare mapcss keys between the base "clear day" style and other styles
  BASEFILE=$DATA_PATH/styles/clear/style-clear/style.mapcss
  BASEKEYS=$(egrep -oh "^[ ]+([^ ]+)" $BASEFILE | sort)
  echo "Validating mapcss files..."
  for item in $DATA_PATH/styles/*/style-*/style.mapcss
  do
    if [[ $item == $BASEFILE ]]
    then
      continue;
    fi
    THISKEYS=$(egrep -oh "^[ ]+([^ ]+)" $item | sort)
    THISDIFF=$(diff --unchanged-line-format="" --old-line-format="- %L" --new-line-format="+ %L" <(echo "$BASEKEYS") <(echo "$THISKEYS"))
    THISWORDS=$(echo "$THISDIFF" | wc -w)
    if [[ $THISWORDS -gt 0 ]]
    then
      echo -e "\nERROR: Missing/different mapcss keys found:"
      echo "--- a/$BASEFILE"
      echo "+++ b/$item"
      echo "$THISDIFF"
      EXITCODE=1
    fi
  done
  if [[ $EXITCODE -eq 0 ]]
  then
    echo -e "\nSuccess! No mismatched mapcss keys found between these files:"
    ls $DATA_PATH/styles/*/style-*/style.mapcss
    echo -e "\nAnd these mapcss keys:"
    echo "$BASEKEYS"
  fi
  set -e
  exit $EXITCODE
fi

function BuildDrawingRules() {
  styleType=$1
  styleName=$2
  suffix=${3-}
  echo "Building drawing rules for style $styleType/$styleName"
  # Cleanup
  rm "$DATA_PATH"/drules_proto$suffix.{bin,txt} || true
  # Run script to build style
  python3 "$OMIM_PATH/tools/kothic/src/libkomwm.py" --txt \
    -s "$DATA_PATH/styles/$styleType/style-$styleName/style.mapcss" \
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
BuildDrawingRules clear  clear _clear
BuildDrawingRules clear  night _dark
BuildDrawingRules outdoors  clear _outdoors_clear
BuildDrawingRules outdoors  night _outdoors_dark
# Keep vehicle style last to produce same visibility.txt & classificator.txt
BuildDrawingRules vehicle  clear _vehicle_clear
BuildDrawingRules vehicle  night _vehicle_dark

# In designer mode we use drules_proto_design file instead of standard ones
cp $OMIM_PATH/data/drules_proto_clear.bin $OMIM_PATH/data/drules_proto_design.bin

echo "Exporting transit colors..."
python3 "$OMIM_PATH/tools/python/transit/transit_colors_export.py" \
  "$DATA_PATH/colors.txt" > /dev/null

echo "Merging default and vehicle styles..."
python3 "$OMIM_PATH/tools/python/stylesheet/drules_merge.py" \
  "$DATA_PATH/drules_proto_clear.bin" "$DATA_PATH/drules_proto_vehicle_clear.bin" \
  "$DATA_PATH/drules_proto.bin.tmp" > /dev/null
echo "Merging in outdoors style..."
python3 "$OMIM_PATH/tools/python/stylesheet/drules_merge.py" \
  "$DATA_PATH/drules_proto.bin.tmp" "$DATA_PATH/drules_proto_outdoors_clear.bin" \
  "$DATA_PATH/drules_proto.bin" "$DATA_PATH/drules_proto.txt" > /dev/null
rm "$DATA_PATH/drules_proto.bin.tmp" || true
