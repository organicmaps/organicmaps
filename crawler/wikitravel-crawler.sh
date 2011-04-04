#!/bin/bash
set -e -u -x
MY_PATH=`dirname $0`

# $MY_PATH/wikitravel-download-lists.sh

# cat wikitravel-redirects-*.html \
#     | $MY_PATH/wikitravel-process-redirects.py \
#     | grep -v Diving_the_Cape_Peninsula \
#     | grep -v '[^\s]*:' \
#     > wikitravel-redirects.json

# cat wikitravel-pages-*.html \
#     | $MY_PATH/wikitravel-process-pages.py \
#     | grep -v Diving_the_Cape_Peninsula \
#     > wikitravel-pages.json

# wc -l wikitravel-pages.json

cat wikitravel-pages.json | $MY_PATH/wikitravel-download-pages.py

cat wikitravel-pages.json | $MY_PATH/wikitravel-process-articles.py

cat wikitravel-pages.json | $MY_PATH/wikitravel-optimize-articles.py

#for file in *.article
#do
#java -jar $MY_PATH/htmlcompressor.jar --remove-intertag-spaces --simple-bool-attr --remove-quotes \
#  --remove-js-protocol --type html -o "${file}.opt" "${file}"
#done

# TODO: Run publisher.
