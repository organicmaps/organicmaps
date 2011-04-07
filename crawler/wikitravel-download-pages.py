#!/opt/local/bin/python
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

    if title.find('_') != -1:
      sys.stderr.write('WARNING! Title contains "_". It will not be found!\n')

    tryCount = 0
    while True:
      try:
        tryCount = tryCount + 1
        remoteFile = urllib2.urlopen(url)
        try:
          data = remoteFile.read();
        finally:
          remoteFile.close()
        break
      except IOError as error:
        sys.stderr.write('Try {0}, error: {1}\n'.format(tryCount, error))
        if tryCount >= 5:
          raise
        else:
          time.sleep(120)

    localFile = open(fileName, 'w')
    localFile.write(data)
    localFile.close()

    time.sleep(1)
