#!/usr/bin/python

import csv
import os.path

from optparse import OptionParser

ID_COLUMN = 1
MIN_PROCESSED_COLUMN = 2
MAX_PROCESSED_COLUMN = 30

def parse_args():
  opt_parser = OptionParser(usage="It's a tool for converting text voice messages from csv to twine input format."
    + "As a source shall be taken a csv file from\n"
    + "https://docs.google.com/spreadsheets/d/1gJsSzFpp2B3xnSx-RjjQ3Do66lQDhCxtfEnQo7Vrkw0/edit#gid=150382014\n"
    + "The output shall be put to omim/sound/tts/sound.txt. As another output file the tool generates languages.txt."
    + "languages.txt contains all available languages in csv.\n"
    + "Example: python %prog path_to_sound.csv path_to_sound.txt path_to_languages.txt", 
    version="%prog 1.0")

  (options, args) = opt_parser.parse_args()

  if len(args) != 3:
    opt_parser.error("Wrong number of arguments.")
  return args


def run():
  args = parse_args()

  input_name = args[0]
  twine_name = args[1]
  languages_name = args[2]

  print("Converting sound.csv to sound.txt (input of twine)")
  if not os.path.isfile(input_name):
    print("Error. CSV file not found. Please check the usage.\n")
    return

  txt_file = open(twine_name, 'w')
  with open(input_name, 'rb') as csvfile:
    txt_file.write('[[sound]]\n')
    csv_reader = csv.reader(csvfile, delimiter=',', quotechar='\n')

    languages = {}
    csv_reader.next()
# A row with language names (like en, ru and so on) is located on the second line.
    language_row = csv_reader.next()
    languages_file = open(languages_name, 'w')
    for idx, lang in enumerate(language_row):
      if (idx >= MIN_PROCESSED_COLUMN 
              and idx < MAX_PROCESSED_COLUMN and lang != ''):
        languages[idx] = lang
        languages_file.write(lang + ' ')
    languages_file.close()
    csv_reader.next()
# Translation follows starting from the 4th line in the table.
    for row in csv_reader:
      if row[ID_COLUMN] != '':
        txt_file.write('  [' + row[ID_COLUMN] + ']\n')
        for column_idx, translation in enumerate(row):
          if (column_idx >= MIN_PROCESSED_COLUMN 
                and column_idx < MAX_PROCESSED_COLUMN and column_idx in languages):
            txt_file.write('    ' + languages[column_idx] + ' = ' + translation + '\n')   
        txt_file.write('\n')

  csvfile.close()
  txt_file.close()
  print('Done. Check ' + twine_name + ' and ' + languages_name + ' for the result.')


if __name__ == "__main__":
  run()
