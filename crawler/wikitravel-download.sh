#!/bin/bash
set -e -u -x
MY_PATH=`dirname $(stat -f %N $PWD"/"$0)`

cat wikitravel-pages-*.html \
    | egrep '<a href=\"/en/.+?bytes]</li>' -o \
    | sed "s@<a href=\"@http://m.wikitravel.org@" \
    | sed "s@\" title=.*</a>.*bytes]</li>@@" \
    | grep -v phrasebook \
    | grep -v "Diving_the_Cape_Peninsula" \
    > wikitravel-urls.txt

# $MY_PATH/download.sh wikitravel-urls.txt "WikiTravel Mobile.html" ./
