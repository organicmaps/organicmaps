#!/opt/local/bin/python
import json
import os.path
import sys
import time
import urllib2

for i, line in enumerate(sys.stdin):
  (_, title, fileName) = json.loads(line)

  url = "http://maps.googleapis.com/maps/api/geocode/json?sensor=false&address=" + urllib2.quote(title.encode("utf-8"), "")
  fileName = fileName + ".google_geocoded"

  if os.path.exists(fileName):
    sys.stderr.write('Skipping existing {0} {1}\n'.format(i, fileName))
  else:
    sys.stderr.write('Downloading {0} {1}\n'.format(i, fileName))

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
          time.sleep(2)

    localFile = open(fileName, 'w')
    localFile.write(data)
    localFile.close()

    time.sleep(36)
