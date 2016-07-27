from __future__ import print_function
import csv
from collections import defaultdict
import sys

if len(sys.argv) <= 1:
    print("""
       * * *

    This script turns a csv file resulting from "translated strings" in the google sheet file into a strings.txt-formated file.

    To use this script, create the translated strings using the google spread-sheet. Go to file -> Download as, and choose csv. Only the currently open sheet will be exported.
    Run this script with the path to the downloaded file as a parameter. The formatted file will be printed to the console.
    Please note, that the order of keys is not (yet) preserved.
       * * *
    """)

    exit(2)

path = sys.argv[1]
resulting_dict = defaultdict(list)

with open(path, mode='r') as infile:
    reader = csv.reader(infile)
    column_names = next(reader)

    for strings in reader:
        for i, string in enumerate(strings):
            if string:
                resulting_dict[column_names[i]].append(string)

for key in column_names:
    if not key:
        continue

    translations = resulting_dict[key]
    print("  {}".format(key))
    for translation in translations:
        print("    {}".format(translation))

    print("")
