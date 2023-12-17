echo "start update benchmark system"
echo "get result from device"
filedate=`date +%d-%m-%y`
filepath=~/Dropbox/results/$filedate
filepath+="_result.txt"
adb pull /mnt/sdcard/MapsWithMe/bench/results.txt $filepath
cd ~/dev/omim/
echo "current work dir : " $PWD
git checkout master
echo "pull upstream"
git fetch upstream
git merge upstream/master
echo "move to android dir"
cd android/MapsWithMePro/
echo "current work dir : " $PWD
echo "rebuild application"
ant clean
ant release
echo "uninstall application from device"
adb uninstall app.organicmaps
echo "install new build"
adb install bin/MapsWithMePro-release.apk
echo "start application"
adb shell am start -n app.organicmaps/app.organicmaps.DownloadResourcesActivity

