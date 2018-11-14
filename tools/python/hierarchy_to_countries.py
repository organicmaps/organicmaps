#!/usr/bin/python
#
# Produces countries.txt (either legacy or new format) from hierarchy.txt
#
# Hierarchy.txt format:
#
# Sample lines:
# Iran;Q794;ir;fa
#  Iran_South;Q794-South
#
# Number of leading spaces mean hierarchy depth. In above case, Iran_South is inside Iran.
# Then follows a semicolon-separated list:
# 1. MWM file name without extension
# 2. Region name template using wikidata Qxxx codes and predefined strings
# 3. Country ISO code (used for flags in the legacy format)
# 4. Comma-separated list of language ISO codes for the region
#
import sys, json, re
from optparse import OptionParser
import os.path
import codecs
import hashlib
import base64

class CountryDict(dict):
  def __init__(self, *args, **kwargs):
    dict.__init__(self, *args, **kwargs)
    self.order = ['id',  'n', 'f', 'v', 'c', 's', 'sha1_base64', 'rs', 'g']

  def __iter__(self):
    for key in self.order:
      if key in self:
        yield key
    for key in dict.__iter__(self):
      if key not in self.order:
        yield key

  def iteritems(self):
    for key in self.__iter__():
      yield (key, self.__getitem__(key))

def get_hash(path, name):
  if path == '0':
    return ''
  filename = os.path.join(path, '{0}.mwm'.format(name)) 
  h = hashlib.sha1()
  with open(filename, 'rb') as f:
    for chunk in iter(lambda: f.read(4096), b""):
      h.update(chunk)
  return base64.b64encode(h.digest())

def get_size(path, name):
  if path == '0':
    return 0
  filename = os.path.join(path, '{0}.mwm'.format(name))
  try:
    return os.path.getsize(filename)
  except OSError:
    sys.stderr.write('Missing file: {0}\n'.format(filename))
    return -1

def collapse_single(root):
  for i in range(len(root['g'])):
    if 'g' in root['g'][i]:
      if len(root['g'][i]['g']) == 1:
        # replace group by a leaf
        if 'c' in root['g'][i]:
          root['g'][i]['g'][0]['c'] = root['g'][i]['c']
        root['g'][i] = root['g'][i]['g'][0]
      else:
        collapse_single(root['g'][i])

def get_name(leaf):
  if 'n' in leaf:
    return leaf['n'].lower()
  else:
    return leaf['id'].lower()

def sort_tree(root):
  root['g'].sort(key=get_name)
  for leaf in root['g']:
    if 'g' in leaf:
      sort_tree(leaf)

parser = OptionParser(add_help_option=False)
parser.add_option('-t', '--target', help='Path to mwm files')
parser.add_option('-h', '--hierarchy', default='hierarchy.txt', help='Hierarchy file')
parser.add_option('--old', help='old_vs_new.csv file')
parser.add_option('--osm', help='borders_vs_osm.csv file')
parser.add_option('-v', '--version', type='int', default=151231, help='Version')
parser.add_option('-o', '--output', help='Output countries.txt file (default is stdout)')
parser.add_option('-m', '--help', action='store_true', help='Display this help')
parser.add_option('--flag', action='store_true', help='Add flags ("c") to countries')
parser.add_option('--lang', action='store_true', help='Add languages ("lang") to countries')
parser.add_option('-l', '--legacy', action='store_true', help='Produce a legacy format file')
parser.add_option('-n', '--names', help='Translations for file names (for legacy format)')
parser.add_option('-s', '--sort', action='store_true', help='Sort leaves by name (useful for legacy)')
(options, args) = parser.parse_args()

if options.help:
  parser.print_help()
  sys.exit(0)

if not os.path.isfile(options.hierarchy):
  parser.error('Hierarchy file is required.')

oldvs = {}
if options.old:
  with open(options.old, 'r') as f:
    for line in f:
      m = re.match(r'(.+?)\t(.+)', line.strip())
      if m:
        if m.group(2) in oldvs:
          oldvs[m.group(2)].append(m.group(1))
        else:
          oldvs[m.group(2)] = [m.group(1)]

vsosm = {}
if options.osm:
  with codecs.open(options.osm, 'r', 'utf-8') as f:
    for line in f:
      m = re.match(r'^(.+?)\t(\d)\t(.+?)$', line.strip())
      if m:
        if m.group(1) in vsosm:
          vsosm[m.group(1)].append(m.group(3))
        else:
          vsosm[m.group(1)] = [m.group(3)]

names = {}
if options.names:
  with codecs.open(options.names, 'r', 'utf-8') as f:
    for line in f:
      pair = [x.strip() for x in line.split('=', 1)]
      if len(pair) == 2 and pair[0] != pair[1]:
        try:
          names[pair[0]] = pair[1]
        except Error:
          sys.stderr.write('Could not read translation for {0}\n'.format(pair[0]))

nameattr = 'n' if options.legacy else 'id'
mwmpath = '0' if not options.target else options.target
stack = [CountryDict({ "v": options.version, nameattr: "World" if options.legacy else "Countries", "g": [] })]
last = None
with open(options.hierarchy, 'r') as f:
  for line in f:
    m = re.match('( *)(.+?)\n', line)
    if m:
      depth = len(m.group(1))
      if last is not None:
        lastd = last['d']
        del last['d']
        if lastd < depth:
          # last is a group
          last['g'] = []
          if options.legacy and 'f' in last:
            del last['f']
          stack.append(last)
        else:
          name = last['f' if 'f' in last else nameattr]
          last['s'] = get_size(mwmpath, name)
          last['sha1_base64'] = get_hash(mwmpath, name)
          if options.legacy:
            last['rs'] = 0
          if last['s'] >= 0:
            stack[-1]['g'].append(last)
      while depth < len(stack) - 1:
        # group ended, add it to higher group
        g = stack.pop()
        if len(g['g']) > 0:
          stack[-1]['g'].append(g)
      items = m.group(2).split(';')
      last = CountryDict({ nameattr: items[0], "d": depth })
      if not options.legacy and items[0] in oldvs:
        last['old'] = oldvs[items[0]]
      if not options.legacy and items[0] in vsosm:
        last['affiliations'] = vsosm[items[0]]
      if (options.legacy or options.flag) and len(items) > 2 and len(items[2]) > 0:
        last['c'] = items[2]
      if options.lang and len(items) > 3 and len(items[3]) > 0:
        last['lang'] = items[3].split(',')
      if options.legacy and items[0] in names:
        last['f'] = last[nameattr]
        last[nameattr] = names[items[0]]

# the last line is always a file
del last['d']
name = last['f' if 'f' in last else nameattr]
last['s'] = get_size(mwmpath, name)
last['sha1_base64'] = get_hash(mwmpath, name)
if options.legacy:
  last['rs'] = 0
if last['s'] >= 0:
  stack[-1]['g'].append(last)
while len(stack) > 1:
  g = stack.pop()
  if len(g['g']) > 0:
    stack[-1]['g'].append(g)

collapse_single(stack[-1])
if options.sort:
  sort_tree(stack[-1])
if options.output:
  with codecs.open(options.output, 'w', 'utf-8') as f:
    json.dump(stack[-1], f, ensure_ascii=True, indent=1)
else:
  print(json.dumps(stack[-1], ensure_ascii=True, indent=1))
