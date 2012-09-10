#!/opt/local/bin/python
import hashlib
import json
import os.path
import sys
import string

for i, line in enumerate(sys.stdin):
  (url, title, fileName1) = json.loads(line)
  page1 = url[27:]
  page2 = page1.replace('(', '%28').replace(')', '%29')
  fileName2 = page2.replace('/', '_') + '_' + hashlib.md5(page2).hexdigest()[:8];
  suffix = '.google_geocoded'
  if os.path.exists(fileName2 + suffix):
    if not os.path.exists(fileName1 + suffix):
      cmd = 'mv "%s" "%s"' % (fileName2 + suffix, fileName1 + suffix)
      print(cmd)
      os.system(cmd)
