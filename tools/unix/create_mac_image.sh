#!/bin/bash

cd ../../../omim-build-release/out/release
umount /Volumes/MapsWithMe
rm MapsWithMe.dmg
~/QtSDK/Desktop/Qt/473/gcc/bin/macdeployqt MapsWithMe.app -dmg
