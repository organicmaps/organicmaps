#!/bin/bash
set -e -x -u

SRC=$1
DST=$2

# Remove old links
rm -rf $DST || true
mkdir $DST || true

files=(copyright.html faq.html resources-ldpi resources-mdpi resources-hdpi resources-xhdpi resources-xxhdpi categories.txt classificator.txt
       types.txt fonts_blacklist.txt fonts_whitelist.txt languages.txt unicode_blocks.txt \ 
       drules_proto.bin drules_proto_dark.bin external_resources.txt packed_polygons.bin countries.txt)

for item in ${files[*]}
do
  ln -s $SRC/$item $DST/$item
done
