#!/usr/bin/python

import os, sys

if len(sys.argv) < 3:
  print 'Creation or resource directories for the languages'
  print 'Usage: {0} <languages.txt> <path_to_dir_with_jsons>'.format(sys.argv[0])
  sys.exit(1)

with open(sys.argv[1], "r") as langs_file:
  for lang in langs_file.read().split():
    new_dir = os.path.join(sys.argv[2], lang + ".json")
    if not os.path.exists(new_dir):
      os.makedirs(new_dir)
