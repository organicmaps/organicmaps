#!/bin/bash

START=$(date +%s)
# get all ids, but not the first line 
devices=$(adb devices | cut -f1 | grep -v List)
if [ -z "$devices" ]; then
    echo "Could not fine any connected devices, try: adb kill-server; adb start-server"
fi
count=$(echo "$devices" | wc -l | tr -d ' ')
echo "Found $count devices and $# action to run"

# run each script with each device's id
for scr_name in $@; do
    for id in $devices; do
        echo "Running $scr_name at $id"
        sh $scr_name $id
    done
done

END=$(date +%s)
DIFF=$(( $END - $START ))
echo "Done in $DIFF seconds"
