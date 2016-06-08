#!/usr/bin/env python
import sys, os.path, json
from mwm import MWM

if len(sys.argv) < 4:
    print('Finds features in an mwm file based on a query')
    print('Usage: {0} <country.mwm> <type> <string>'.format(sys.argv[0]))
    print('')
    print('Type:')
    print('  t for inside types ("t hwtag" will find all hwtags-*)')
    print('  et for exact type ("et shop" won\'t find shop-chemist)')
    print('  n for names, case-sensitive ("n Starbucks" for all starbucks)')
    sys.exit(1)

typ = sys.argv[2].lower()
find = sys.argv[3].decode('utf-8')

mwm = MWM(open(sys.argv[1], 'rb'))
mwm.read_header()
mwm.read_types(os.path.join(os.path.dirname(sys.argv[0]), '..', '..', '..', 'data', 'types.txt'))
for feature in mwm.iter_features():
    found = False
    if typ == 'n' and 'name' in feature['header']:
        for value in feature['header']['name'].values():
            if find in value:
                found = True
    elif typ in ('t', 'et'):
        for t in feature['header']['types']:
            if t == find:
                found = True
            elif typ == 't' and find in t:
                found = True
    if found:
        print(json.dumps(feature, ensure_ascii=False))
