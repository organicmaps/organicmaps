#!/usr/bin/python

import sys, csv

if len(sys.argv) < 4:
  print 'Converting Google Docs translation CSV to strings.txt format'
  print 'Translation valid only for page "Small mwm names" in document:'
  print 'https://docs.google.com/spreadsheets/d/1dKhThUersOGoUyOCx5OfGMXEjs1LBmTFcIsTc_nGxZc/edit#gid=1022107578'
  print 'Usage: {0} <source.csv> <output.txt> <languages.txt>'.format(sys.argv[0])
  sys.exit(1)

def GetName(n):
    return n[:n.index(' ')]

languages = set()

with open(sys.argv[2], 'w') as o:
  with open(sys.argv[1], 'r') as f:
    reader = csv.reader(f)
    reader.next()
    header = reader.next()
    for row in reader:
      print >> o, '[{0}]'.format(row[0])
      for i in range(1, len(header)):
        languages.add(GetName(format(header[i])))
        print >> o, '{0}{1}'.format(header[i], row[i])
      print >> o

with open(sys.argv[3], 'w') as l:
  print >> l, " ".join(languages)
