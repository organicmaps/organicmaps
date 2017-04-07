#!/usr/bin/env python
import sys
import mwm

if len(sys.argv) < 3:
    print('Finds an OSM object for a given feature id.')
    print('Usage: {} <mwm.osm2ft> <ftid>'.format(sys.argv[0]))
    sys.exit(1)

with open(sys.argv[1], 'rb') as f:
    ft2osm = mwm.read_osm2ft(f, ft2osm=True)

code = 0
type_abbr = {'n': 'node', 'w': 'way', 'r': 'relation'}
for ftid in sys.argv[2:]:
    ftid = int(ftid)
    if ftid in ft2osm:
        print('https://www.openstreetmap.org/{}/{}'.format(
            type_abbr[ft2osm[ftid][0]],
            ft2osm[ftid][1]
        ))
    else:
        print('Could not find osm id for feature {}'.format(ftid))
        code = 2
sys.exit(code)
