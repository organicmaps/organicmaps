#!/bin/bash
set -x -u

SRC=../../data
DST=../../android/assets

# Remove old links
rm -rf $DST
mkdir $DST

files=(about.html basic_ldpi.skn basic_mdpi.skn basic_hdpi.skn basic_xhdpi.skn categories.txt classificator.txt 
       types.txt fonts_blacklist.txt fonts_whitelist.txt languages.txt \
       symbols_ldpi.png symbols_mdpi.png symbols_hdpi.png symbols_xhdpi.png unicode_blocks.txt \ 
       visibility.txt drules_proto.txt drules_proto.bin external_resources.txt packed_polygons.bin)

for item in ${files[*]}
do
  ln -s $SRC/$item $DST/$item
done

# Separate case for World and countries list files without search support
ln -s $SRC/countries.txt.nosearch $DST/countries.txt
