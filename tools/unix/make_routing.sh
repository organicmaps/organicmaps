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

$PV ../../../omim-maps/$2.osm.bz2 | bzip2 -d | $GENERATOR_TOOL --osrm_file_name=../../../omim-maps/$2.osrm --output=$2
