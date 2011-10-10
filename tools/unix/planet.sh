#!/bin/bash
################################################
# Builds whole planet in /media/ssd/common dir #
################################################

# At least "set -e -u" should always be here, not just for debugging!
# "set -x" is useful to see what is going on.
set -e -u -x

# global params
LIGHT_NODES=false
PROCESSORS=8
BZIP="pbzip2 -d"

# displays usage and exits
function Usage {
  echo ''
  echo "Usage: $0 [path_to_data_folder_with_classsif_and_planet.osm.bz2] [optional_path_to_intermediate_data]"
  exit 0
}

# for parallel builds
function forky() {
  local num_par_procs
  if [[ -z $1 ]] ; then
    num_par_procs=2
  else
    num_par_procs=$1
  fi
  while [[ $(jobs | wc -l) -ge $num_par_procs ]] ; do
    sleep 1
  done
}

if [ $# -lt 1 ]; then
  Usage
fi

DATA_PATH=$1

# set up necessary Windows MinGW settings
#if [ ${WINDIR+1} ]; then
#fi

# check if we have QT in PATH
#if [ ! `which qmake` ]; then
#  echo 'You should add your qmake binary into the PATH. This can be done in 2 ways:'
#  echo '  1. Set it temporarily by executing: export PATH=/c/qt/your_qt_dir/bin:$PATH'
#  echo '  2. Set it permanently by adding export... string above to your ~/.bashrc'
#  echo 'Hint: for second solution you can type from git bash console: notepad ~/.bashrc'
#  exit 0
#fi

# determine script path
MY_PATH=`dirname $0`

# find generator_tool
IT_PATHS_ARRAY=(  "$MY_PATH/../../../omim-build-release/out/release/generator_tool" \
                  "$MY_PATH/../../out/release/generator_tool" )

for i in {0..1}; do
  if [ -x ${IT_PATHS_ARRAY[i]} ]; then
    GENERATOR_TOOL=${IT_PATHS_ARRAY[i]}
    echo TOOL: $GENERATOR_TOOL
    break
  fi
done

if [[ ! -n $GENERATOR_TOOL ]]; then
  echo 'No generator_tool found, please build omim-build-release or omim/out/release'
  echo ""
  Usage
fi

PLANET_OSM_BZ2=$DATA_PATH/planet.osm.bz2
COASTS_OSM_BZ2=$DATA_PATH/coastlines.osm.bz2

if ! [ -f $PLANET_OSM_BZ2 ]; then
  echo "Can't open file $PLANET_OSM_BZ2, did you forgot to specify dataDir?"
  echo ""
  Usage
fi

if ! [ -f $COASTS_OSM_BZ2 ]; then
  echo "Can't open file $COASTS_OSM_BZ2, did you forgot to specify dataDir?"
  echo ""
  Usage
fi

TMPDIR=$DATA_PATH/intermediate_data/

if [ $# -ge 2 ]; then
  TMPDIR=$2/
fi

if ! [ -d $TMPDIR ]; then
  mkdir -p $TMPDIR
fi

PV="cat"
if [ `which pv` ]
then
  PV=pv
fi

# 1st pass - preprocess coastlines
$PV $COASTS_OSM_BZ2 | $BZIP | $GENERATOR_TOOL -intermediate_data_path=$TMPDIR \
    -use_light_nodes=true \
    -preprocess_xml

# 2nd pass - generate temporary coastlines file in the intermediate dir
$BZIP $COASTS_OSM_BZ2 | $GENERATOR_TOOL -intermediate_data_path=$TMPDIR \
    -use_light_nodes=true -make_coasts

# 3rd pass - preprocess planet
$PV $PLANET_OSM_BZ2 | $BZIP | $GENERATOR_TOOL -intermediate_data_path=$TMPDIR \
    -use_light_nodes=$LIGHT_NODES \
    -preprocess_xml

# wait until 2nd pass is finished
#wait

# 4nd pass - paralleled in the code
$PV $PLANET_OSM_BZ2 | $BZIP | $GENERATOR_TOOL -intermediate_data_path=$TMPDIR \
  -use_light_nodes=$LIGHT_NODES -split_by_polygons \
  -generate_features -generate_world \
  -data_path=$DATA_PATH -emit_coasts

# 5rd pass - do in parallel
# but separate exceptions for wolrd files to finish them earlier
$GENERATOR_TOOL -data_path=$DATA_PATH -generate_geometry -generate_index -output=World &
$GENERATOR_TOOL -data_path=$DATA_PATH -generate_geometry -generate_index -output=WorldCoasts &
for file in $DATA_PATH/*.mwm; do
  if [[ "$file" == *minsk-pass*  ]]; then
    continue
  fi
  if [[ "$file" == *World*  ]]; then
    continue
  fi
  filename=$(basename "$file")
  extension="${filename##*.}"
  filename="${filename%.*}"
  $GENERATOR_TOOL -data_path=$DATA_PATH -generate_geometry -generate_index -output="$filename" &
  forky $PROCESSORS
done

wait
