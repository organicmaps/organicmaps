#!/bin/bash

FILES=( World.mwm \
        WorldCoasts.mwm \
        01_dejavusans.ttf \
        02_droidsans-fallback.ttf \
        03_jomolhari-id-a3d.ttf \
        04_padauk.ttf \
        05_khmeros.ttf \
        06_code2000.ttf )

for file in ${FILES[*]}; do
  stat -c "%n %s" $file
done
