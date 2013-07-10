#!/bin/bash

LITE=com.mapswithme.maps
if [ -z $1 ]; then
    adb uninstall $LITE
else
    adb -s "$1" uninstall $LITE
fi
