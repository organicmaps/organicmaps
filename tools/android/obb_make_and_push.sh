./obb_make.sh

adb shell "mkdir /mnt/sdcard/Android/obb/com.mapswithme.maps.pro"
adb push fonts.obb /mnt/sdcard/Android/obb/com.mapswithme.maps.pro/fonts.obb
adb push world.obb /mnt/sdcard/Android/obb/com.mapswithme.maps.pro/world.obb

rm fonts.obb
rm world.obb
