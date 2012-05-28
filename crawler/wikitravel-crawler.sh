#!/bin/bash
set -e -u -x
MY_PATH=`dirname $0`

$MY_PATH/wikitravel-download-lists.sh

cat wikitravel-redirects-*.html \
     | python $MY_PATH/wikitravel-process-redirects.py \
     | grep -v '[^\s]*:' \
     > wikitravel-redirects.json

cat wikitravel-pages-*.html \
     | python $MY_PATH/wikitravel-process-pages.py \
     > wikitravel-pages.json

echo "Total pages:"
wc -l wikitravel-pages.json

cat wikitravel-pages.json | python $MY_PATH/wikitravel-download-pages.py

cat wikitravel-pages.json | python $MY_PATH/wikitravel-geocode-yahoo.py

cat wikitravel-pages.json | python $MY_PATH/wikitravel-geocode-google.py

cat wikitravel-pages.json | python $MY_PATH/wikitravel-process-articles.py

cat wikitravel-pages.json | python $MY_PATH/wikitravel-optimize-articles.py

$MY_PATH/extract-image-urls.sh wikitravel-images.urls

# TODO: Run publisher.
