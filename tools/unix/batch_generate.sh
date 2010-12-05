#!/bin/sh

if [ $# -lt 1 ]; then
  echo Usage: $0 path_to_osm_bz2_files_directory  
  exit 0
fi

for file in $1/*.osm.bz2; do
  COUNTRY=${file/.osm.bz2/}
  country=${COUNTRY/$1\//}
  
  ./make_index_of.sh $country $1
done

