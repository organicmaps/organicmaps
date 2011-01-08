#!/bin/bash
################################################
# Builds whole planet in /media/ssd/common dir #
################################################

# At least "set -e -u" should always be here, not just for debugging!
# "set -x" is useful to see what is going on.
set -e -u -x

# global params
LIGHT_NODES=false
PROCESSORS=4
DATA_PATH=../../data


# displays usage and exits
function Usage {
  echo ''
  echo "Usage: $0 [path_to_data_folder_with_classsif_and_planet.osm.bz2] [bucketing_level] [optional_path_to_intermediate_data]"
  echo "Note, that for coastlines bucketing_level should be 0"
  echo "Planet squares size is (2^bucketing_level x 2^bucketing_level)"
  echo "If optional intermediate path is given, only second pass will be executed"
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

if [ $# -lt 2 ]; then
  Usage
fi

BUCKETING_LEVEL=$2

# set up necessary Windows MinGW settings
#if [ ${WINDIR+1} ]; then
#fi

# check if we have QT in PATH
if [ ! `which qmake` ]; then
  echo 'You should add your qmake binary into the PATH. This can be done in 2 ways:'
  echo '  1. Set it temporarily by executing: export PATH=/c/qt/your_qt_dir/bin:$PATH'
  echo '  2. Set it permanently by adding export... string above to your ~/.bashrc'
  echo 'Hint: for second solution you can type from git bash console: notepad ~/.bashrc'
  exit 0
fi

# determine script path
MY_PATH=`dirname $0`

# find indexer_tool
IT_PATHS_ARRAY=(  "$MY_PATH/../../../omim-build-release/out/release/indexer_tool" \
                  "$MY_PATH/../../out/release/indexer_tool" )

for i in {0..1}; do
  if [ -x ${IT_PATHS_ARRAY[i]} ]; then
    INDEXER_TOOL=${IT_PATHS_ARRAY[i]}
    echo TOOL: $INDEXER_TOOL
    break
  fi
done

if [[ ! -n $INDEXER_TOOL ]]; then
  echo 'No indexer_tool found, please build omim-build-release or omim/out/release'
  echo ""
  Usage
fi

OSM_BZ2=$1/planet.osm.bz2

if ! [ -f $OSM_BZ2 ]; then
  echo "Can't open file $OSM_BZ2, did you forgot to specify dataDir?"
  echo ""
  Usage
fi

TMPDIR=$1/intermediate_data/

if [ $# -ge 3 ]; then
  TMPDIR=$3/
fi

if ! [ -d $TMPDIR ]; then
  mkdir -p $TMPDIR
fi

PV="cat"
if [ `which pv` ]
then
  PV=pv
fi

# skip 1st pass if intermediate data path was given
#if [ $# -lt 3 ]; then
#  # 1st pass - not paralleled
#  $PV $OSM_BZ2 | bzip2 -d | $INDEXER_TOOL --intermediate_data_path=$TMPDIR \
#    --use_light_nodes=$LIGHT_NODES \
#    --preprocess_xml
#fi

# 2nd pass - not paralleled
#$PV $OSM_BZ2 | bzip2 -d | $INDEXER_TOOL --intermediate_data_path=$TMPDIR \
#  --use_light_nodes=$LIGHT_NODES --bucketing_level=$BUCKETING_LEVEL \
#  --generate_features --worldmap_max_zoom=5

# 3rd pass - do in parallel
for file in $DATA_PATH/*.mwm; do
  if [ $file != "minsk-pass"  ]; then
    filename=$(basename $file)
    extension=${filename##*.}
    filename=${filename%.*}
    $INDEXER_TOOL --generate_geometry --sort_features --output=$filename &
    forky $PROCESSORS
  fi
done

wait

# 4th pass - do in parallel
for file in $DATA_PATH/*.mwm; do
  if [ $file != "minsk-pass"  ]; then
    filename=$(basename $file)
    extension=${filename##*.}
    filename=${filename%.*}
    $INDEXER_TOOL --generate_index --output=$filename &
    forky $PROCESSORS
  fi
done

wait
