#!/bin/bash
set -e -x -u

SRC=../../data
DST=../../android/assets

# Remove old links
rm $DST/*

files=(01_dejavusans.ttf 02_wqy-microhei.ttf 03_jomolhari-id-a3d.ttf 04_padauk.ttf \
       05_khmeros.ttf 06_code2000.ttf World.mwm WorldCoasts.mwm about-travelguide-iphone.html \
       basic.skn basic_highres.skn categories.txt classificator.txt types.txt countries.txt \
       drawing_rules.bin fonts_blacklist.txt fonts_whitelist.txt languages.txt \
       maps.update symbols_24.png symbols_48.png unicode_blocks.txt visibility.txt \
       drules_proto.txt)

for item in ${files[*]}
do
  ln -s $SRC/$item $DST/$item
done
