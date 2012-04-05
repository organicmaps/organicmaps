#!/opt/local/bin/python

import os.path
import sys
import time
import urllib2

USER_AGENT="MapsWithMe Scraper <info@mapswithme.com>"

def ScrapeUrl(url, fileName, delay, info, maxTryCount = 5, userAgent = USER_AGENT):
  if os.path.exists(fileName):
    sys.stderr.write('Skipping existing {0} {1}\n'.format(info, fileName))
  else:
    sys.stderr.write('Downloading {0} {1} {2} \n'.format(info, fileName, url))

    tryCount = 0
    while True:
      try:
        tryCount = tryCount + 1
        remoteFile = urllib2.urlopen(urllib2.Request(url, None, { "User-Agent" : userAgent }))
 
        try:
          data = remoteFile.read()
        finally:
          remoteFile.close()
        break
      except IOError as error:
        sys.stderr.write('Try {0}, error: {1}\n'.format(tryCount, error))
        if tryCount >= maxTryCount:
          raise
        else:
          time.sleep(delay)

    localFile = open(fileName, 'w')
    localFile.write(data)
    localFile.close()

    time.sleep(delay)
