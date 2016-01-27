#!/usr/bin/python
import sys, os.path, random
from mwm import MWM

if len(sys.argv) < 2:
  print 'Dumps some MWM structures.'
  print 'Usage: {0} <country.mwm>'.format(sys.argv[0])
  sys.exit(1)

mwm = MWM(open(sys.argv[1], 'rb'))
mwm.read_types(os.path.join(os.path.dirname(sys.argv[0]), '..', '..', '..', 'data', 'types.txt'))
print 'Tags:'
for tag, value in mwm.tags.iteritems():
  print '  {0:<8}: offs {1:9} len {2:8}'.format(tag, value[0], value[1])
print 'Version:', mwm.read_version()
print 'Header:', mwm.read_header()
print 'Metadata count:', len(mwm.read_metadata())

cross = mwm.read_crossmwm()
if cross:
  print 'Outgoing points:', len(cross['out']), 'incoming:', len(cross['in'])
  print 'Outgoing regions:', set(cross['neighbours'])

print 'Sample features:'
count = 5
probability = 1.0 / 1000
for feature in mwm.iter_features():
  if random.random() < probability:
    print feature
    count -= 1
    if count <= 0:
      break
