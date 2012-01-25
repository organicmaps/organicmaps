#!/bin/bash
set -x -u

SRC=../../data
DST=../../android/assets

# Remove old links
rm $DST/*

files=(01_dejavusans.ttf 02_wqy-microhei.ttf 03_jomolhari-id-a3d.ttf 04_padauk.ttf \
       05_khmeros.ttf 06_code2000.ttf World.mwm WorldCoasts.mwm about-travelguide-iphone.html \
       basic_ldpi.skn basic_mdpi.skn basic_hdpi.skn basic_xhdpi.skn categories.txt classificator.txt 
       types.txt countries.txt fonts_blacklist.txt fonts_whitelist.txt languages.txt \
       symbols_ldpi.png symbols_mdpi.png symbols_hdpi.png symbols_xhdpi.png unicode_blocks.txt \ 
       visibility.txt drules_proto.txt drules_proto.bin)

for item in ${files[*]}
do
  ln -s $SRC/$item $DST/$item
done
