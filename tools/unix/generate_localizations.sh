#!/bin/bash

set -e -u -x

# TODO: Add "--untagged --tags android" when tags are properly set.
./tools/twine/twine generate-all-string-files ./strings.txt ./android/res/
./tools/twine/twine generate-all-string-files ./strings.txt ./iphone/Maps/

# Delete dummy string files
rm ./android/res/values-sw600dp/strings.xml
rm ./android/res/values-sw720dp-land/strings.xml
rm ./android/res/values-sw720dp/strings.xml
rm ./android/res/values-v11/strings.xml
