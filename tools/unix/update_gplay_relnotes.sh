#!/bin/bash

# Concatenates Android release notes in all languages into a single output format
# suitable to upload to Google Play to update existing notes.

GPLAY_NOTES=android/app/src/google/play/release-notes/*/default.txt

for x in $(ls $GPLAY_NOTES); do
  l=$(basename $(dirname $x));
  echo "<"$l">";
  cat $x;
  echo "</"$l">";
done
