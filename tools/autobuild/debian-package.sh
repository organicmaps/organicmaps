#!/bin/bash

send_buildlogs () {
    pushd /home/osm/
    for i in build-service/*build; do tail -n 70 $i | mail -a $i -s $i dev@mapswithme.com; done
    rm build-service/*build
    popd
}

drop_buildlogs () {
    pushd /home/osm/
    rm build-service/*build
    popd
}

send_beta () {
    mv $1 $2
    git log --since="`date -d "3 day ago"`" | mail -a $2 -s "[beta]$2" dev@mapswithme.com
}



cd /home/osm/build-service
rm *.build
cd omim
git remote update
git checkout -f origin/master
#git pull
git reset --hard HEAD
git submodule update --init
export PATH=/usr/lib/ccache:$PATH
# For correct QT version
export PATH=/home/osm/Qt5.2.0/5.2.0/gcc_64/bin:$PATH
export QT_HOME=/home/osm/Qt5.2.0/5.2.0/gcc_64

cd tools/android
python set_up_android.py --sdk /home/osm/android/adt-bundle-linux-x86_64-20130917/sdk --ndk /home/osm/android/android-ndk-r9

BUILD_DATE=`date +"%Y%m%d-%H%M"`-`hostname`-`git rev-parse HEAD | cut -c-10`

# Android Pro
cd ../../android/MapsWithMePro
cat AndroidManifest.xml | sed s/android:versionName=\"/android:versionName=\"auto-$BUILD_DATE/g > AndroidManifest-new.xml; mv AndroidManifest-new.xml AndroidManifest.xml
NDK_CCACHE=ccache timeout -k 8h 4h ant production > ~/build-service/MapsWithMePro-production-android.build
cp bin/MapsWithMePro-production.apk ~/build-service
if [ "$?" = 0 ]; then 
    drop_buildlogs
    send_beta bin/MapsWithMePro-production.apk MapsWithMePro-production-$BUILD_DATE.apk $BUILD_DATE
#else 
#    send_buildlogs
fi


# Android Lite
cd ../../android/MapsWithMeLite
cat AndroidManifest.xml | sed s/android:versionName=\"/android:versionName=\"auto-$BUILD_DATE/g > AndroidManifest-new.xml; mv AndroidManifest-new.xml AndroidManifest.xml
NDK_CCACHE=ccache timeout -k 8h 4h ant production > ~/build-service/MapsWithMeLite-production-android.build
cp bin/MapsWithMeLite-production.apk ~/build-service
if [ "$?" = 0 ]; then 
    drop_buildlogs
    send_beta bin/MapsWithMeLite-production.apk MapsWithMeLite-production-$BUILD_DATE.apk $BUILD_DATE
#else 
#    send_buildlogs
fi

drop_buildlogs

# Ubuntu Raring AMD64
ARCH=amd64 DIST=raring timeout -k 8h 4h pdebuild --debbuildopts "-b -j9"
if [ "$?" = 0 ]; then drop_buildlogs; else send_buildlogs; fi

# Ubuntu Raring i386
ARCH=i386  DIST=raring timeout -k 8h 4h pdebuild --debbuildopts "-b -j9"
if [ "$?" = 0 ]; then drop_buildlogs; else send_buildlogs; fi

#ARCH=armel DIST=stable timeout -k 8h 4h pdebuild --debbuildopts "-b -j9"
#if [ "$?" = 0 ]; then drop_buildlogs; else send_buildlogs; fi

#ARCH=armhf DIST=raring timeout -k 8h 4h pdebuild --debbuildopts "-b -j9"
#if [ "$?" = 0 ]; then drop_buildlogs; else send_buildlogs; fi

