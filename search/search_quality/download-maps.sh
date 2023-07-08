#!/bin/bash

# Downloads all maps necessary for learning to rank to the current
# directory.

ALL=
VERSION=
BASE="http://direct.mapswithme.com/direct"

display_usage() {
    echo "Usage: $0 -v [version] -a -h"
    echo "    -v  version of maps to download"
    echo "    -a  download all maps of the specified version"
    echo "    -c  continue getting partially-downloaded files; new files on the server must be exactly the same as the ones in the previous attempt"
    echo "    -h  display this message"
}

while getopts ":acv:h" opt
do
    case "$opt" in
        a) ALL=1
           ;;
        c) RESUME_PARTIAL="-c"
           ;;
        v) VERSION="$OPTARG"
           ;;
        h) display_usage
           exit -1
           ;;
        \?) echo "Invalid option: -$OPTARG" 1>&2
            display_usage
            exit -1
            ;;
        :) echo "Option -$OPTARG requires an argument" 1>&2
           display_usage
           exit -1
           ;;
    esac
done

if [ -z "$VERSION" ]
then
    echo "Version of maps is not specified." 1>&2
    exit -1
fi

if ! curl "$BASE/" 2>/dev/null |
        sed -n 's/^.*href="\(.*\)\/".*$/\1/p' |
        grep -v "^../$" | grep -q "$VERSION"
then
    echo "Invalid version: $VERSION" 1>&2
    exit -1
fi

NAMES=("Australia_Brisbane.mwm"
       "Belarus_Hrodna*.mwm"
       "Belarus_Minsk*.mwm"
       "Canada_Ontario_London.mwm"
       "Canada_Quebek_Montreal.mwm"
       "Germany_*.mwm"
       "Russia_*.mwm"
       "UK_England_*.mwm"
       "US_California_*.mwm"
       "US_Maryland_*.mwm")

DIR="$BASE/$VERSION"

if [ "$ALL" ]
then
    echo "Downloading all maps..."

    files=$(curl "$DIR/" 2>/dev/null | sed -n 's/^.*href="\(.*\.mwm\)".*$/\1/p')

    set -e
    set -x
    for file in $files
    do
        wget $RESUME_PARTIAL -np -nd "$DIR/$file"
    done
else
    echo "Downloading maps..."

    set -e
    set -x
    for name in ${NAMES[@]}
    do
        wget $RESUME_PARTIAL -r -np -nd -A "$name" "$DIR/"
    done
fi
