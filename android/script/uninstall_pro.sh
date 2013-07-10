#!/bin/bash

PRO=com.mapswithme.maps.pro
if [ -z $1 ]; then
    adb uninstall $PRO
else
    adb -s "$1" uninstall $PRO
fi
