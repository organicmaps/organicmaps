#!/bin/bash

pushd

cd ../../../omim-build-release/out/release
umount /Volumes/MapsWithMe
rm MapsWithMe.dmg
/Developer/QtSDK/Desktop/Qt/4.8.0/gcc/bin/macdeployqt MapsWithMe.app -dmg

popd
