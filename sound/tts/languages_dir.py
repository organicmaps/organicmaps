#!/usr/bin/python

import os

from optparse import OptionParser


def parse_args():
  opt_parser = OptionParser(usage="This tool creates directories for languages for twine."
    + "All directory names shall be listed in languages file with space separator.\n"
    + "Example: python %prog path_to_language_file.txt path_to_dir_with_json_files", 
    version="%prog 1.0")

  (options, args) = opt_parser.parse_args()

  if len(args) != 2:
    opt_parser.error("Wrong number of arguments.")
  return args


def run():
  args = parse_args()

  langs_name = args[0]
  json_dir_name = args[1]

  print "Creating directories for languages in json."
  with open(langs_name, 'r') as langs_file:
    for lang in langs_file.read().split():
      new_dir = json_dir_name + lang + '.json'
      if not os.path.exists(new_dir):
        os.makedirs(new_dir)

    langs_file.close()


if __name__ == "__main__":
  run()
