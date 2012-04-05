#!/opt/local/bin/python
import scrapelib

import argparse
import json
import os.path
import sys
import time
import urllib2

argParser = argparse.ArgumentParser(description = 'Download Wikipedia for a given locale.')
argParser.add_argument("--locale", required = True)
argParser.add_argument("--minlat", type=float, default=-1000)
argParser.add_argument("--maxlat", type=float, default=1000)
argParser.add_argument("--minlon", type=float, default=-1000)
argParser.add_argument("--maxlon", type=float, default=1000)
ARGS = argParser.parse_args()

for i, line in enumerate(sys.stdin):
  (itemId, lat, lon, itemType, title) = json.loads(line)
  
  if lat >= ARGS.minlat and lat <= ARGS.maxlat and lon >= ARGS.minlon and lon <= ARGS.maxlon:
    fileName = urllib2.quote(title.encode("utf-8"), " ()") + ".html"
    url = "http://{0}.wikipedia.org/w/index.php?curid={1}&useformat=mobile".format(ARGS.locale, itemId)

    if title.find('_') != -1:
      sys.stderr.write('WARNING! Title contains "_". It will not be found!\n')

    scrapelib.ScrapeUrl(url, fileName, 1, i)
