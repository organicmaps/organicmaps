#!/usr/bin/env python3
import argparse
import json
import logging
import sys
import os
import urllib.request
try:
    from lxml import etree
except ImportError:
    import xml.etree.ElementTree as etree


TYPES = [k.split('=') for k in [
    'sponsored=partner1',
    'sponsored=partner2',
    'sponsored=partner3',
    'sponsored=partner4',
    'sponsored=partner5',
    'sponsored=partner6',
    'sponsored=partner7',
    'sponsored=partner8',
    'sponsored=partner9',
    'sponsored=partner10',
    'sponsored=partner11',
    'sponsored=partner12',
    'sponsored=partner13',
    'sponsored=partner14',
    'sponsored=partner15',
    'sponsored=partner16',
    'sponsored=partner17',
    'sponsored=partner18',
    'sponsored=partner19',
    'sponsored=partner20',
    'event=fc2018_city',
    'event=fc2018',
]]
OSM_CACHE = os.path.join(os.path.dirname(sys.argv[0]), 'osm_cache.json')


def query_osm(osm_type, osm_id):
    if os.path.exists(OSM_CACHE):
        with open(OSM_CACHE, 'r') as f:
            cache = json.load(f)
    else:
        cache = {}
    k = '{}{}'.format(osm_type, osm_id)
    if k in cache:
        coord = cache[k]
        return (coord[1], coord[0])

    OSM_API_SERVER = 'https://api.openstreetmap.org/api/0.6'
    try:
        r = urllib.request.urlopen('{}/{}/{}{}'.format(
            OSM_API_SERVER, osm_type, osm_id, '/full' if osm_type != 'node' else ''))
    except OSError as e:
        logging.warn('Could not download %s %s: %s', osm_type, osm_id, e)
        return None
    xml = etree.parse(r)
    nodes = {}
    for nd in xml.findall('node'):
        nodes[nd.get('id')] = (float(nd.get('lat')), float(nd.get('lon')))
    ways = {}
    for way in xml.findall('way'):
        coord = [0, 0]
        count = 0
        for nd in way.findall('nd'):
            count += 1
            for i in range(len(coord)):
                coord[i] += nodes[nd.get('ref')][i]
        ways[way.get('id')] = [coord[0] / count, coord[1] / count]

    coord = None
    if osm_type == 'node':
        coord = nodes[str(osm_id)]
    elif osm_type == 'way':
        coord = ways[str(osm_id)]
    else:
        for el in xml.findall(osm_type):
            if el.get('id') == str(osm_id):
                coord = [0, 0]
                count = 0
                for m in el.findall('member'):
                    if m.get('type') == 'node' and m.get('ref') in nodes:
                        count += 1
                        for i in range(len(coord)):
                            coord[i] += nodes[m.get('ref')][i]
                    elif m.get('type') == 'way' and m.get('ref') in ways:
                        count += 1
                        for i in range(len(coord)):
                            coord[i] += ways[m.get('ref')][i]
                if count > 0:
                    coord = [coord[0] / count, coord[1] / count]
    if coord is None:
        return None

    cache[k] = coord
    with open(OSM_CACHE, 'w') as f:
        json.dump(cache, f)
    return (coord[1], coord[0])


def get_coord(tags):
    lat = tags.pop('lat', None)
    lon = tags.pop('lon', None)
    if lat is None or lon is None:
        return None
    return [float(lon), float(lat)]


def get_type(tags):
    global TYPES
    for t in TYPES:
        if tags.get(t[0]) == t[1]:
            return t[min(len(t)-1, 2)]
    return None


def make_feature(coord, ftype, tags):
    global type_to_colour, COLOURS
    colour = None
    if ftype in type_to_colour:
        colour = type_to_colour[ftype]
    else:
        if type_to_colour:
            i = max(type_to_colour.values())
        else:
            i = -1
        if i < len(COLOURS) - 1:
            colour = i + 1
            type_to_colour[ftype] = colour
    if colour is not None:
        tags['marker-color'] = COLOURS[colour]

    return {
        'type': 'Feature',
        'geometry': {
            'type': 'Point',
            'coordinates': coord,
        },
        'properties': tags
    }


def ok_to_print(options, ftype):
    if ftype is not None and options.missing:
        return False
    if options.types:
        t = [s.strip() for s in options.types.split(',')]
        if ftype not in t:
            return False
    return True


parser = argparse.ArgumentParser(
    description='This script converts a mixed_whatever.txt file to a geojson.')
parser.add_argument('txt', help='Path to mixed_nodes.txt or mixed_tags.txt.')
parser.add_argument('-d', '--duplicates', action='store_true',
                    help='If specified, geojson will contain data only for duplicate '
                    'elements in mixed_tags.txt.')
parser.add_argument('-t', '--types',
                    help='Comma-separated list of types to include.')
parser.add_argument('-m', '--missing', action='store_true',
                    help='Include only items with type missing from the built-in list.')
options = parser.parse_args()

logging.basicConfig(level=logging.INFO, format='%(message)s')

COLOURS = [
    '#a6cee3',
    '#1f78b4',
    '#b2df8a',
    '#33a02c',
    '#fb9a99',
    '#e31a1c',
    '#fdbf6f',
    '#ff7f00',
    '#cab2d6',
    '#6a3d9a',
    '#ffff99',
    '#b15928',
]
type_to_colour = {}

features = []
is_nodes = 'nodes' in options.txt
tags = {}
coord = None
seen = {}
line_no = 0
with open(options.txt, 'r') as f:
    for line in f:
        line_no += 1
        if line.strip()[:1] == '#':
            continue
        if is_nodes:
            if len(line.strip()) == 0 and tags:
                coord = get_coord(tags)
                if coord is None:
                    logging.warn('No coordinate for line %s, type %s', line_no, get_type(tags))
                    tags = {}
            else:
                kv = [s.strip() for s in line.split('=', 1)]
                if len(kv) == 2 and len(kv[0]) > 0:
                    tags[kv[0]] = kv[1]
        else:
            parts = [s.strip() for s in line.split(',')]
            if len(parts) >= 3 and parts[1].isdigit():
                kv = [s.split('=', 1) for s in parts[2:]]
                tags = {k[0].strip(): k[1].strip() for k in kv}
                # Check for duplicates
                if options.duplicates:
                    k = (parts[0], parts[1])
                    if k not in seen:
                        seen[k] = True
                        tags = {}
                        continue
                # Got OSM object, query its coordinates
                coord = query_osm(parts[0], parts[1])
                if coord is None:
                    logging.warn('No coordinate for line %s, type %s', line_no, get_type(tags))
        if coord:
            ftype = get_type(tags)
            if ok_to_print(options, ftype):
                features.append(make_feature(coord, ftype, tags))
            coord = None
            tags = {}

if is_nodes and tags:
    coord = get_coord(tags)
    ftype = get_type(tags)
    if coord and ok_to_print(options, ftype):
        features.append(make_feature(coord, ftype, tags))

print(json.dumps({'type': 'FeatureCollection', 'features': features},
                 ensure_ascii=False, indent=1))
