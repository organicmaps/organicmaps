#!/bin/bash

#1
adb -s S557026e32038 uninstall com.mapswithme.maps.pro
adb -s S557026e32038 install bin/MapsWithMePro-debug.apk
adb -s S557026e32038 shell am start -a android.intent.action.MAIN -n com.mapswithme.maps.pro/com.mapswithme.maps.DownloadResourcesActivity
#2
adb -s 4df1615c2383596f uninstall com.mapswithme.maps.pro
adb -s 4df1615c2383596f install -r bin/MapsWithMePro-debug.apk
adb -s 4df1615c2383596f shell am start -a android.intent.action.MAIN -n com.mapswithme.maps.pro/com.mapswithme.maps.DownloadResourcesActivity
#3
adb -s B0CB060325140SCN uninstall com.mapswithme.maps.pro
adb -s B0CB060325140SCN install -r bin/MapsWithMePro-debug.apk
adb -s B0CB060325140SCN shell am start -a android.intent.action.MAIN -n com.mapswithme.maps.pro/com.mapswithme.maps.DownloadResourcesActivity
