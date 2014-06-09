#!/bin/bash
set -x -u

SRC=../../data
DST=$1

# Remove old links
rm -rf $DST
mkdir $DST

files=(about.html resources-ldpi resources-mdpi resources-hdpi resources-xhdpi resources-xxhdpi categories.txt classificator.txt 
       types.txt fonts_blacklist.txt fonts_whitelist.txt languages.txt unicode_blocks.txt \ 
       drules_proto.bin external_resources.txt packed_polygons.bin countries.txt)

for item in ${files[*]}
do
  ln -s $SRC/$item $DST/$item
done
