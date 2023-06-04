#!/usr/bin/python
import sys, re

RE_STAT = re.compile(r'(?:\d+\. )?([\w:|-]+?)\|: size = (\d+); count = (\d+); length = ([0-9.e+-]+) m; area = ([0-9.e+-]+) m.\s*')
def parse_and_add(data, line):
  m = RE_STAT.match(line)
  if m:
    data[m.group(1)] = int(m.group(3))

if len(sys.argv) < 3:
  print('This tool compares type_statistics output for feature sizes')
  print('Usage: {0} <output_new> <output_old> [threshold_in_%]'.format(sys.argv[0]))
  sys.exit(0)

data1 = {}
with open(sys.argv[2], 'r') as f:
  for line in f:
    parse_and_add(data1, line)
data2 = {}
with open(sys.argv[1], 'r') as f:
  for line in f:
    parse_and_add(data2, line)

threshold = (int(sys.argv[3]) if len(sys.argv) > 3 else 100) / 100.0 + 1
min_diff = 40

for k in data1:
  v1 = int(data1[k])
  if k in data2:
    v2 = int(data2[k])
    if v1 == 0 or v2 == 0 or max(v1, v2) / float(min(v1, v2)) > threshold and abs(v1 - v2) > min_diff:
      print('{0}: {1} to {2}'.format(k, v1, v2))
  elif v1 > min_diff:
    print('- not found: {0}, {1}'.format(k, v1))
