set -e -u -x
MY_PATH=`echo $0 | grep -o '.*/'`
GENERATOR_TOOL=$MY_PATH../../../omim-build-$1/out/$1/generator_tool

TMPDIR=../../../omim-indexer-tmp/

if ! [ -d $TMPDIR ]
then 
    mkdir $TMPDIR
fi

$GENERATOR_TOOL --make_routing --make_cross_section --osrm_file_name=../../../omim-maps/$2.osrm --output=$2
