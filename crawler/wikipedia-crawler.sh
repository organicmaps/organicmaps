#!/bin/bash
set -e -u -x
MY_PATH=`dirname $0`
WIKI_LOCALE="en"
NOW=$(date +"%Y-%m-%d-%H-%M")

wget http://wikilocation.org/mysqldumps/$WIKI_LOCALE.sql.gz

gunzip $WIKI_LOCALE.sql.gz

cat $WIKI_LOCALE.sql | grep "INSERT INTO" > $WIKI_LOCALE.inserts

cat $WIKI_LOCALE.inserts | sed 's/INSERT INTO .* VALUES (/[/g' | sed 's/),(/]\n[/g' | sed 's/);/]/g' | sed "s/','/\",\"/g" | sed "s/,'/,\"/g" | sed "s/']/\"]/g" | sed "s/\\\'/\\'/g" > $WIKI_LOCALE-pages.json

cat $WIKI_LOCALE-pages.json | python $MY_PATH/wikipedia-download-pages.py --locale=$WIKI_LOCALE --minlat=45.8 --maxlat=47.83 --minlon=5.93 --maxlon=10.54 2>&1 | tee wikipedia-download.log.$NOW

# TODO: Run publisher.
