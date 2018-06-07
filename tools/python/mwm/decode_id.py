#!/usr/bin/env python
import sys
import mwm
import re

if len(sys.argv) < 2:
    print('This script unpacks maps.me OSM id to an OSM object link.')
    print('Usage: {} {<id> | <url>}'.format(sys.argv[0]))
    sys.exit(1)

if sys.argv[1].isdigit():
    osm_id = mwm.unpack_osmid(int(sys.argv[1]))
    type_abbr = {'n': 'node', 'w': 'way', 'r': 'relation'}
    print('https://www.openstreetmap.org/{}/{}'.format(
        type_abbr[osm_id[0]], osm_id[1]))
else:
    m = re.search(r'/(node|way|relation)/(\d+)', sys.argv[1])
    if m:
        oid = int(m.group(2))
        if m.group(1) == 'node':
            oid |= mwm.OsmIdCode.NODE
        elif m.group(1) == 'way':
            oid |= mwm.OsmIdCode.WAY
        elif m.group(1) == 'relation':
            oid |= mwm.OsmIdCode.RELATION
        print(oid)
    else:
        print('Unknown parameter format')
        sys.exit(2)
