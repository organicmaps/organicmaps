#!/bin/bash
##########################################
#
#  This script changes version code and version name in AndroidManifest.xml.
#  Must be run from omim/android directory.
#
##########################################

while :
do
    case $1 in
	-h)
	    echo "Usage: change_version.sh -vn <version_name> -vc <version_code>"
	    exit 0
	    ;;
	-vn)
	    VERSION_NAME=$2
	    echo "Version name: $VERSION_NAME"
	    shift 2
	    ;;
	-vc)
	    VERSION_CODE=$2
	    echo "Version code: $VERSION_CODE"
	    shift 2
	    ;;
	*)  # no more options
	    break
	    ;;
    esac
done

set -e -u -x

AM_DIRS=(MapsWithMeLite MapsWithMeLite.Samsung MapsWithMePro MapsWithMeTest)
for DIR in ${AM_DIRS[*]}
do
    sed -i '' "s/versionName=\"[0-9\.]*\"/versionName=\"${VERSION_NAME}\"/g" \
	${DIR}/AndroidManifest.xml
    sed -i '' "s/versionCode=\"[0-9]*\"/versionCode=\"${VERSION_CODE}\"/g" \
	${DIR}/AndroidManifest.xml
done
