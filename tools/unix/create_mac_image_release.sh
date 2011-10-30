#!/bin/bash

pushd

cd ../../../omim-build-release/out/release
umount /Volumes/MapsWithMe
rm MapsWithMe.dmg
~/QtSDK/Desktop/Qt/474/gcc/bin/macdeployqt MapsWithMe.app -dmg

popd
