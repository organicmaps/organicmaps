#!/usr/bin/python
import json
import re
import sys

input = sys.stdin.read()
redirects = re.findall('<li><a .*? title="(.+?)">.*?</a>.*?<a .*? title="(.+?)">.*?</a></li>', input)
for redirect in redirects:
  print json.dumps(redirect)
