#!/usr/bin/python
import json
import os.path
import sys
import time
import urllib2

for i, line in enumerate(sys.stdin):
  (url, title, fileName) = json.loads(line)
  if os.path.exists(fileName):
    sys.stderr.write('Skipping existing {0} {1}\n'.format(i, fileName))
  else:
    sys.stderr.write('Downloading {0} {1}\n'.format(i, fileName))

    remoteFile = urllib2.urlopen(url)
    data = remoteFile.read();
    remoteFile.close()

    localFile = open(fileName, 'w')
    localFile.write(data)
    localFile.close()

    time.sleep(1)
