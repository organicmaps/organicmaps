adb pull /mnt/sdcard/MapsWithMe/bench/results.txt ../results.txt
cd ..
python tk_results_viewer.py config.txt results.txt