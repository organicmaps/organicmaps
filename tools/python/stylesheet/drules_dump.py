#!/usr/bin/python
# Dumps hashes of protobuffed drules
import sys, re
import itertools
import drules_struct_pb2

if len(sys.argv) < 2:
  print 'Usage: {} <drules_proto.bin>'.format(sys.argv[0])
  sys.exit(1)

drules = drules_struct_pb2.ContainerProto()
drules.ParseFromString(open(sys.argv[1]).read())
result = []

for elem in drules.cont:
  if not elem.element:
    continue
  for el in elem.element:
    zoom = el.scale
    if zoom <= 0:
      continue
    for l in itertools.chain(el.lines, (el.caption, el.path_text, el.circle, el.area, el.symbol)):
      if l.HasField('priority'):
        l.ClearField('priority')
    result.append('{} z{}: {}'.format(elem.name, zoom, re.sub(r'[\r\n\s]+', ' ', str(el))))

for line in sorted(result):
  print line
