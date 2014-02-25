set -e -u -x
MY_PATH=`echo $0 | grep -o '.*/'`
GENERATOR_TOOL=$MY_PATH../../../omim-build-$1/out/$1/generator_tool

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

# Common part.
$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $GENERATOR_TOOL --preprocess_xml=true --use_light_nodes=true --intermediate_data_path=$TMPDIR

# 1. Generate coastlines.
#$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $GENERATOR_TOOL --make_coasts=true --use_light_nodes=true --intermediate_data_path=$TMPDIR
#$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $GENERATOR_TOOL --use_light_nodes=true --generate_world=true --split_by_polygons=true --generate_features=true --emit_coasts=true --intermediate_data_path=$TMPDIR

#$GENERATOR_TOOL --generate_geometry=true --generate_index=true --generate_search_index=true --output=$2
#$GENERATOR_TOOL --generate_geometry=true --generate_index=true --generate_search_index=true --output=World
#$GENERATOR_TOOL --generate_geometry=true --generate_index=true --output=WorldCoasts

# 2. Generate country from --output.
$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $GENERATOR_TOOL --use_light_nodes=true --generate_features=true --generate_geometry=true --generate_index=true --generate_search_index=true --intermediate_data_path=$TMPDIR --output=$2 --address_file_name=$2.addr
