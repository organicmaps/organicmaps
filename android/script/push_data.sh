#!/bin/bash

DATA=../tests/data
DST=/sdcard/MapsWithMe

# clean all
adb -s $1 shell rm $DST/*.kmz
adb -s $1 shell rm $DST/*.kml

# copy files
for file in $(find $DATA -type f) ; do
    echo $file
    adb -s $1 push $file $DST
done
