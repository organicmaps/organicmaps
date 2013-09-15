#!/bin/bash
set -x -u

SRC=../../../data
DST=$1

files=(World.mwm WorldCoasts.mwm \
       01_dejavusans.ttf 02_wqy-microhei.ttf 03_jomolhari-id-a3d.ttf 04_padauk.ttf 05_khmeros.ttf 06_code2000.ttf)

for item in ${files[*]}
do
  ln -s -f $SRC/$item $DST/$item
done
