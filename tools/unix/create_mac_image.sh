#!/bin/bash
set -u -x
TARGET=${1:-release}
(
  cd ../../../omim-build-$TARGET/out/$TARGET
  umount /Volumes/MapsWithMe
  rm MapsWithMe.dmg
  macdeployqt MapsWithMe.app -dmg
)
