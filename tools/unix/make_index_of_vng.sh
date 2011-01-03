set -e -u -x
MY_PATH=`echo $0 | grep -o '.*/'`
INDEXER_TOOL=$MY_PATH../../../omim-build/out/$1/indexer_tool

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

$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $INDEXER_TOOL --generate_intermediate_data=true --generate_final_data=false --use_light_nodes=true --generate_index=false --intermediate_data_path=$TMPDIR
$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $INDEXER_TOOL --generate_intermediate_data=false --generate_final_data=true --use_light_nodes=true --generate_index=true --sort_features=true --intermediate_data_path=$TMPDIR --output=$2 --bucketing_level=0
