#!/bin/bash
################################################
# Cool script for building mwm files           #
################################################

set -e -u -x

# displays usage and exits
function Usage {
  echo ''
  echo "Usage: $0 countryName [dataDir] [omim-build-suffix] [heavyNodes]"
  echo 'omim-build-suffix is "release" or "debug" or just a shadow build folder name'
  echo 'heavyNodes if present, enables Big Nodes Temp File Mode for continent/planet OSM'
  exit 0
}

if [ $# -lt 1 ]; then
  Usage
fi

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
# check if user specified build suffix parameter
if [ $# -ge 3 ]; then
  IT_PATHS_ARRAY=(  "$MY_PATH/../../../omim-build-$3/out/$3/generator_tool" \
                    "$MY_PATH/../../../$3/out/$3/generator_tool" \
                    "$MY_PATH/../../../omim-$3/out/$3/generator_tool" \
                    "stub-for-cycle" )
else
  IT_PATHS_ARRAY=(  "$MY_PATH/../../../build-omim/out/release/generator_tool"
                    "$MY_PATH/../../../omim-build-release/out/release/generator_tool" \
                    "$MY_PATH/../../../omim-release/out/release/generator_tool"
                    "$MY_PATH/../../../omim-build-debug/out/debug/generator_tool" \
                    "$MY_PATH/../../../omim-debug/out/debug/generator_tool" )
fi
for i in {0..4}; do
  if [ -x ${IT_PATHS_ARRAY[i]} ]; then
    GENERATOR_TOOL=${IT_PATHS_ARRAY[i]}
    echo TOOL: $GENERATOR_TOOL
    break
  fi
done

if [[ ! -n $GENERATOR_TOOL ]]; then
  echo 'No generator_tool found, please build omim-build-release or omim-build-debug'
  echo 'or specify omim-build-[suffix] or [shadow directory] as 3rd script parameter'
  Usage
fi

# determine data directory
if [ $# -ge 2 ]; then
  DATADIR=$2
else
  DATADIR=$MY_PATH/../../data
fi

OSM_BZ2=$DATADIR/$1.osm.bz2

if ! [ -f $OSM_BZ2 ]; then
  echo "Can't open file $OSM_BZ2, did you forgot to specify dataDir?"
  Usage
fi

if [ -d /media/ssd/tmp ]; then
  TMPDIR=/media/ssd/tmp/$LOGNAME/
else
  TMPDIR=/tmp/
fi

if ! [ -d $TMPDIR ]; then
  mkdir -p $TMPDIR
fi

PV="cat"
if [ `which pv` ]
then
  PV=pv
fi

if [ $# -ge 4 ]; then
  LIGHT_NODES=false
else
  LIGHT_NODES=true
fi

$PV $OSM_BZ2 | bzip2 -d | $GENERATOR_TOOL -intermediate_data_path=$TMPDIR \
  -use_light_nodes=$LIGHT_NODES \
  -preprocess_xml

$PV $OSM_BZ2 | bzip2 -d | $GENERATOR_TOOL -intermediate_data_path=$TMPDIR \
  -use_light_nodes=$LIGHT_NODES \
  -generate_features -generate_geometry -generate_index \
  -generate_search_index -output=$1
