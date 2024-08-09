#!/bin/sh

SCRIPT_DIR=$(dirname -- "${BASH_SOURCE[0]}")
OUTPUT_FILE="$SCRIPT_DIR/../../data/faq.html"

echo "Downloading latest FAQ page from organicmaps.app website ..."
curl -s -L -f -o $OUTPUT_FILE https://organicmaps.app/faq/embedded-faq

res=$?
if test "$res" != "0"; then
   echo "The curl command failed with: $res"
else
   echo "Success!"
fi
