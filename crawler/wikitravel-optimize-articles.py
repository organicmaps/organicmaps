#!/opt/local/bin/python
import json
import os
import re
import string
import sys

myPath = os.path.dirname(os.path.realpath(__file__))

for i, line in enumerate(sys.stdin):
  (url, title, fileBase) = json.loads(line)
  fileName = fileBase + '.article'
  outFileName = fileName + '.opt'
  if os.path.exists(outFileName):
    sys.stderr.write('Skipping existing {0} {1}\n'.format(i, fileName))
  else:
    sys.stderr.write('Optimizing {0} {1}\n'.format(i, fileName))
    assert 0 == os.system('java -jar {myPath}/htmlcompressor.jar '
                 '--remove-intertag-spaces --simple-bool-attr --remove-quotes '
                  '--remove-js-protocol --type html '
                '-o {outFileName} {fileName}'
                 .format(myPath = myPath, fileName = fileName, outFileName = outFileName))
