#!/bin/bash

pushd

cd ../../../omim-build-debug/out/debug
umount /Volumes/MapsWithMe
rm MapsWithMe.dmg
~/QtSDK/Desktop/Qt/474/gcc/bin/macdeployqt MapsWithMe.app -dmg -use-debug-libs

popd
