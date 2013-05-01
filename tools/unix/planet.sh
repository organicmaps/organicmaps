#!/bin/bash
#################################################
# Builds whole planet on linux server in Zurich #
#################################################

# At least "set -e -u" should always be here, not just for debugging!
# "set -x" is useful to see what is going on.
set -e -u -x

# global params
PLANET_FILE="$HOME/toolchain/v2/data/planet-latest.o5m"
COASTS_FILE="$HOME/toolchain/v2/data/coastlines-latest.o5m"

FILTER_TOOL="$HOME/toolchain/v2/git/osmctools/osmfilter"
CONVERT_TOOL="$HOME/toolchain/v2/git/osmctools/osmconvert"

CREATE_COASTS_FILE="$HOME/toolchain/v2/git/osmctools/osmfilter $PLANET_FILE --keep= --keep-ways=\"natural=coastline\" -o=$COASTS_FILE"
OSM_TO_PIPE="$HOME/toolchain/v2/git/osmctools/osmconvert $PLANET_FILE"

# displays usage and exits
function Usage {
  echo ''
  echo "Usage: $0 [--full|--generate|--continue]"
  echo "  --full       if specified, planet file will be updated and clean generation will start"
  echo "  --generate   if specified, clean generation will start"
  echo "  --continue   if specified, previously generated intermediate data and coasts will be used for current generation"
  exit 0
}

# for parallel builds
function forky() {
  local num_par_procs=`nproc`
  while [[ $(jobs | wc -l) -ge $num_par_procs ]] ; do
    sleep 1
  done
}

if [ $# -lt 1 ]; then
  Usage
fi

# determine script path
MY_PATH=`dirname $0`

# find generator_tool
IT_PATHS_ARRAY=( "$MY_PATH/../../../omim-build-production/out/production/generator_tool" \
                 "$MY_PATH/../../../omim-production/out/production/generator_tool" \
                 "$MY_PATH/../../out/production/generator_tool" \
                 "$MY_PATH/../../../omim-build-release/out/release/generator_tool" \
                 "$MY_PATH/../../../omim-release/out/release/generator_tool" \
                 "$MY_PATH/../../out/release/generator_tool" )

for i in {0..5}; do
  if [ -x ${IT_PATHS_ARRAY[i]} ]; then
    GENERATOR_TOOL=${IT_PATHS_ARRAY[i]}
    echo TOOL: $GENERATOR_TOOL
    break
  fi
done

if [[ ! -n $GENERATOR_TOOL ]]; then
  echo "No generator_tool found in ${IT_PATHS_ARRAY[*]}"
  echo ""
  Usage
fi

DATA_PATH="$MY_PATH/../../data"
INTDIR=$DATA_PATH/intermediate_data/
INTCOASTSDIR=${INTDIR}coasts/

if ! [ -d $INTDIR ]; then
  mkdir -p $INTDIR
fi

if ! [ -d $INTCOASTSDIR ]; then
  mkdir -p $INTCOASTSDIR
fi

# Cycle while coasts will be merged only if --full key was used
if [[ $1 == "--full" ]]; then
  FAIL_ON_COASTS="true"
else
  FAIL_ON_COASTS="false"
fi

# If exit code is 255 and FAIL_ON_COASTS is true this means total fail or that coasts are not merged
function merge_coasts() {
  # Strip coastlines from the planet to speed up the process
  $FILTER_TOOL $PLANET_FILE --keep= --keep-ways="natural=coastline" -o=$COASTS_FILE
  # Preprocess coastlines to separate intermediate directory
  $CONVERT_TOOL $COASTS_FILE | $GENERATOR_TOOL -intermediate_data_path=$INTCOASTSDIR -use_light_nodes=true -preprocess_xml
  # Generate temporary coastlines file in the coasts intermediate dir
  $CONVERT_TOOL $COASTS_FILE | $GENERATOR_TOOL -intermediate_data_path=$INTCOASTSDIR -use_light_nodes=true -make_coasts -fail_on_coasts=$FAIL_ON_COASTS
}

# Loop while coasts will be correctly merged
if [[ $1 == "--full" ]]; then

  EXIT_CODE=255
  while [[ $EXIT_CODE != 0 ]]; do

    # Get fresh data from OSM server
    pushd  ~/toolchain/v2
       bash update_planet.sh
    popd

    # Do not exit on error
    set +e
    merge_coasts
    set -e
    EXIT_CODE=$?
    if [[ $EXIT_CODE != 0 ]]; then
      echo "Will try fresh coasts again in 40 minutes..."
      sleep 2400
    fi
  done
else
  if [[ $1 == "--generate" ]]; then
    # Just merge coasts and continue
    # Do not exit on error
    set +e
    merge_coasts
    set -e
  fi
fi

# need link to generated coastlines file
ln -s -f $INTCOASTSDIR/WorldCoasts.mwm.tmp $INTDIR/WorldCoasts.mwm.tmp

if [[ $1 == "--generate" || $1 == "--full" ]]; then
  # 1st pass, run in parallel - preprocess whole planet to speed up generation if all coastlines are correct
  $CONVERT_TOOL $PLANET_FILE | $GENERATOR_TOOL -intermediate_data_path=$INTDIR -use_light_nodes=false -preprocess_xml
fi

if [[ $1 == "--generate" || $1 == "--continue" || $1 == "--full" ]]; then
  # 2nd pass - paralleled in the code
  $CONVERT_TOOL $PLANET_FILE | $GENERATOR_TOOL -intermediate_data_path=$INTDIR \
    -use_light_nodes=false -split_by_polygons \
    -generate_features -generate_world \
    -data_path=$DATA_PATH -emit_coasts

  # 3rd pass - do in parallel
  # but separate exceptions for world files to finish them earlier
  PARAMS="-data_path=$DATA_PATH -generate_geometry -generate_index"
  $GENERATOR_TOOL $PARAMS -output=World &
  $GENERATOR_TOOL $PARAMS -output=WorldCoasts &

  PARAMS_WITH_SEARCH="$PARAMS -generate_search_index"
  # additional exceptions for long-generated countries
  $GENERATOR_TOOL $PARAMS_WITH_SEARCH "-output=Russia_Far Eastern" &
  for file in $DATA_PATH/*.mwm.tmp; do
    if [[ "$file" == *minsk-pass*  ]]; then
      continue
    fi
    if [[ "$file" == *World*  ]]; then
      continue
    fi
    if [[ "$file" == *Russia_Far\ Eastern* ]]; then
      continue
    fi
    filename=$(basename "$file")
    extension="${filename##*.}"
    filename="${filename%.*.*}"
    $GENERATOR_TOOL $PARAMS_WITH_SEARCH -output="$filename" &
    forky
  done

  wait

  # Save World without search index
  cp "$DATA_PATH/World.mwm" "$DATA_PATH/World.mwm.nosearch"
  # Generate search index for World
  $GENERATOR_TOOL -data_path=$DATA_PATH -generate_search_index -output=World

fi
