#!/bin/bash

MWM_DATA_PATH=/sdcard/MapsWithMe
if [ -z $1 ]; then
    adb shell rm -r $MWM_DATA_PATH
else
    adb -s "$1" shell rm -r $MWM_DATA_PATH
fi
