#!/bin/bash

# we depend on generate_version.sh script in the same directory!!!

VERSION_SCRIPT_PATH=`dirname $0`
VERSION_STRING=`bash $VERSION_SCRIPT_PATH/generate_version.sh $1 $2 $3`

echo "Altering bundle version in $4"

perl -i -pe "s/\@VERSION\@/$VERSION_STRING/" $4