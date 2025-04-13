#!/bin/bash

adb pull /mnt/sdcard/MapsWithMe/results.txt ../results.txt
cd ..
python tk_results_viewer.py config.txt results.txt
