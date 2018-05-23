#!/usr/bin/env python3
import sys
import mwm

if len(sys.argv) < 2:
    print('This script unpacks maps.me OSM id to an OSM object link.')
    print('Usage: {} <id>'.format(sys.argv[0]))

osm_id = mwm.unpack_osmid(int(sys.argv[1]))
type_abbr = {'n': 'node', 'w': 'way', 'r': 'relation'}
print('https://www.openstreetmap.org/{}/{}'.format(
    type_abbr[osm_id[0]], osm_id[1]))
