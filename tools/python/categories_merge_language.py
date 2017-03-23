#!/usr/bin/env python3
import sys
import os

if len(sys.argv) < 2:
    print('Merges some language in categories.txt with English')
    print('Usage: {} <lang> [path_to_categories.txt]'.format(sys.argv[0]))
    sys.exit(1)

lang = sys.argv[1]
if len(sys.argv) > 2:
    path = sys.argv[2]
else:
    path = os.path.join(os.path.dirname(sys.argv[0]), '..', '..', 'data', 'categories.txt')

with open(path, 'r') as f:
    first = True
    langs = []
    trans = None

    def flush_langs():
        for l in langs:
            if trans and l[0] == 'en':
                parts = l[1].split('|')
                parts[0] = '{} - {}'.format(parts[0], trans)
                l[1] = '|'.join(parts)
            print(':'.join(l))

    for line in f:
        if len(line.strip()) == 0 or line[0] == '#':
            if langs:
                flush_langs()
                langs = []
                trans = None
            print(line.strip())
            first = True
        elif first:
            print(line.strip())
            first = False
        else:
            if ':' not in line:
                raise Exception('Line {} is not a translation line'.format(line))
            l = line.strip().split(':')
            langs.append(l)
            if l[0] == lang:
                trans = l[1].split('|')[0]
                if trans[0].isdigit():
                    trans = trans[1:]
                if trans[0] == '^':
                    trans = trans[1:]
    flush_langs()
