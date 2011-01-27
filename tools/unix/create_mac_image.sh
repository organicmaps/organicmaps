#!/bin/bash

cd ../../../omim-build-release/out/release
umount /Volumes/MapsWithMe
rm MapsWithMe.dmg
macdeployqt MapsWithMe.app -dmg
