set -e -u -x
MY_PATH=`echo $0 | grep -o '.*/'`
INDEXER_TOOL=$MY_PATH../../../omim-build-$1/out/$1/indexer_tool

TMPDIR=../../../omim-indexer-tmp/

if ! [ -d $TMPDIR ]
then 
    mkdir $TMPDIR
fi

PV="cat"
if [ `which pv` ]
then
  PV=pv
fi

$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $INDEXER_TOOL --preprocess_xml=true --use_light_nodes=true --intermediate_data_path=$TMPDIR

$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $INDEXER_TOOL --use_light_nodes=true --generate_features=true --generate_geometry=true --generate_index=true --sort_features=true --intermediate_data_path=$TMPDIR --output=$2 --bucketing_level=0
