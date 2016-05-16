#!/bin/bash

# Downloads all maps necessary for learning to rank to the current
# directory.

case $# in
    1) VERSION="$1"
       ;;
    *) echo "Usage: $0 version" 2>&1
       exit -1
       ;;
esac

BASE="http://direct.mapswithme.com/direct/$VERSION/"
NAMES=("Australia_Brisbane.mwm"
       "Belarus_Minsk*.mwm"
       "Germany_*.mwm"
       "Russia_*.mwm"
       "UK_England_*.mwm"
       "US_California_*.mwm" "US_Maryland_*.mwm")

set -e
set -x
for name in ${NAMES[@]}
do
    wget -r -np -nd -A "$name" "$BASE"
done
