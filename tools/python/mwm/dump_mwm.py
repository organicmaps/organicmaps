#!/usr/bin/python
import sys, os.path, random
import json
from mwm import MWM

if len(sys.argv) < 2:
    print('Dumps some MWM structures.')
    print('Usage: {0} <country.mwm>'.format(sys.argv[0]))
    sys.exit(1)

mwm = MWM(open(sys.argv[1], 'rb'))
mwm.read_types(os.path.join(os.path.dirname(sys.argv[0]), '..', '..', '..', 'data', 'types.txt'))
print('Tags:')
tvv = sorted([(k, v[0], v[1]) for k, v in mwm.tags.items()], key=lambda x: x[1])
for tv in tvv:
    print('  {0:<8}: offs {1:9} len {2:8}'.format(tv[0], tv[1], tv[2]))
v = mwm.read_version()
print('Format: {0}, version: {1}'.format(v['fmt'], v['date'].strftime('%Y-%m-%d %H:%M')))
print('Header: {0}'.format(mwm.read_header()))
print('Region Info: {0}'.format(mwm.read_region_info()))
print('Metadata count: {0}'.format(len(mwm.read_metadata())))

cross = mwm.read_crossmwm()
if cross:
    print('Outgoing points: {0}, incoming: {1}'.format(len(cross['out']), len(cross['in'])))
    print('Outgoing regions: {0}'.format(set(cross['neighbours'])))

# Print some random features using reservoir sampling
count = 5
sample = []
for i, feature in enumerate(mwm.iter_features()):
    if i < count:
        sample.append(feature)
    elif random.randint(0, i) < count:
        sample[random.randint(0, count-1)] = feature

print('Feature count: {0}'.format(i))
print('Sample features:')
for feature in sample:
    print(json.dumps(feature, ensure_ascii=False))
