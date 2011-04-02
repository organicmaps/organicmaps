#!/usr/bin/python
import hashlib
import json
import re
import string
import sys

input = sys.stdin.read()
pages = re.findall('<a href="/en/(.+?)" title="(.+?)".+?bytes]</li>', input)
for page in pages:
  print json.dumps(("http://m.wikitravel.org/en/" + page[0],
                    page[1],
                    string.replace(page[0], '/', '_') + '_' + hashlib.md5(page[0]).hexdigest()[:8]))
