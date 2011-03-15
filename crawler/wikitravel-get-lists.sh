#!/bin/bash
set -e -u -x
MY_PATH=`dirname $(stat -f %N $PWD"/"$0)`

LONGPAGES_URL="http://wikitravel.org/wiki/en/index.php?title=Special:Longpages"
REDIRECTS_URL="http://wikitravel.org/wiki/en/index.php?title=Special:Listredirects"

# Get all pages.
wget $LONGPAGES_URL"&limit=5000&offset=0"     -O wikitravel-pages-0.html && sleep 10s
wget $LONGPAGES_URL"&limit=5000&offset=5000"  -O wikitravel-pages-1.html && sleep 10s
wget $LONGPAGES_URL"&limit=5000&offset=10000" -O wikitravel-pages-2.html && sleep 10s
wget $LONGPAGES_URL"&limit=5000&offset=15000" -O wikitravel-pages-3.html && sleep 10s

# Get all redirects.
wget $REDIRECTS_URL"&limit=5000&offset=0"     -O wikitravel-redirects-0.html && sleep 10s
wget $REDIRECTS_URL"&limit=5000&offset=5000"  -O wikitravel-redirects-1.html && sleep 10s
wget $REDIRECTS_URL"&limit=5000&offset=10000" -O wikitravel-redirects-2.html && sleep 10s
wget $REDIRECTS_URL"&limit=5000&offset=15000" -O wikitravel-redirects-3.html && sleep 10s
