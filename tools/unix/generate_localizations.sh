#!/bin/bash

set -e -u -x

# TODO: Add "--untagged --tags android" when tags are properly set.
./tools/twine/twine generate-all-string-files ./strings.txt ./android/res/
./tools/twine/twine generate-all-string-files ./strings.txt ./iphone/Maps/

