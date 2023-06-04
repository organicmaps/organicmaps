#!/usr/bin/env python3
import sys
import os

path = os.path.join(os.path.dirname(sys.argv[0]), '..', '..', 'data', 'categories.txt')
if len(sys.argv) < 2:
    print('Merges some language in categories.txt with English')
    print('Usage: {} <lang> [path_to_categories.txt]'.format(sys.argv[0]))
    print('Default path to categories: {}'.format(path))
    if not os.path.exists(path):
        print('Warning: default path to categories.txt will fail')
    sys.exit(1)

lang = sys.argv[1]
if len(sys.argv) > 2:
    path = sys.argv[2]

with open(path, 'r') as f:
    langs = []
    trans = None

    def flush_langs():
        for lang in langs:
            if trans and l[0] == 'en':
                parts = lang[1].split('|')
                parts[0] = '{} - {}'.format(parts[0], trans)
                lang[1] = '|'.join(parts)
            print(':'.join(lang))

    for line in map(str.strip, f):
        if len(line) == 0 or line[0] == '#':
            if langs:
                flush_langs()
                langs = []
                trans = None
            print(line)
        elif not langs:
            print(line)
        else:
            if ':' not in line:
                raise Exception('Line {} is not a translation line'.format(line))
            l = line.split(':')
            langs.append(l)
            if l[0] == lang:
                trans = l[1].split('|')[0]
                if trans[0].isdigit():
                    trans = trans[1:]
                if trans[0] == '^':
                    trans = trans[1:]
    flush_langs()
