#!/usr/bin/python

from __future__ import print_function
from optparse import OptionParser

import csv
import os.path

ID_COLUMN = 1
MIN_PROCESSED_COLUMN = 2
MAX_PROCESSED_COLUMN = 30


def parse_args():
  opt_parser = OptionParser(usage="It's a tool for converting text voice messages from csv to twine input format."
    + "As a source shall be taken a csv file from\n"
    + "https://docs.google.com/spreadsheets/d/1gJsSzFpp2B3xnSx-RjjQ3Do66lQDhCxtfEnQo7Vrkw0/edit#gid=150382014\n"
    + "The output shall be put to omim/sound/tts/sound.txt. As another output file the tool generates languages.txt."
    + "languages.txt contains all available languages in csv.\n"
    + "Notice. The script exchanges all non-breaking spaces with spaces.\n"
    + "Example: python %prog path_to_sound.csv path_to_sound.txt path_to_languages.txt", 
    version="%prog 1.0")

  (options, args) = opt_parser.parse_args()

  if len(args) != 3:
    opt_parser.error("Wrong number of arguments.")
  return args


def nbsp_to_spaces(str):
  return str.replace('\xc2\xa0', ' ')


def run():
  csv_name, twine_name, languages_name = parse_args()

  print("Converting sound.csv to sound.txt (input of twine)")
  if not os.path.isfile(csv_name):
    print("Error. CSV file not found. Please check the usage.\n")
    return

  with open(twine_name, 'w') as twine_file:
    with open(csv_name, 'rb') as csv_file:
      twine_file.write('[[sound]]\n')
      csv_reader = csv.reader(csv_file, delimiter=',', quotechar='\n')

      languages = dict()
      csv_reader.next()
# A row with language names (like en, ru and so on) is located on the second line.
      language_row = csv_reader.next()
      with open(languages_name, 'w') as languages_file:
        for idx, lang in enumerate(language_row):
          if (MIN_PROCESSED_COLUMN <= idx < MAX_PROCESSED_COLUMN and lang):
            languages[idx] = lang
            languages_file.write(lang + ' ')

      csv_reader.next()
# Translation follows starting from the 4th line in the table.
      for row in csv_reader:
        if row[ID_COLUMN]:
          twine_file.write('  [{section}]\n'.format(section = nbsp_to_spaces(row[ID_COLUMN])))
          for column_idx, translation in enumerate(row):
            if (MIN_PROCESSED_COLUMN <= column_idx < MAX_PROCESSED_COLUMN and column_idx in languages.keys()):
              twine_file.write('    {lang} = {trans}\n'.format(lang = languages[column_idx], 
                trans = nbsp_to_spaces(translation)))
          twine_file.write('\n')

  print('Done. Check {twine} and {lang} for the result.\n'.format(twine = twine_name, lang = languages_name))


if __name__ == "__main__":
  run()
