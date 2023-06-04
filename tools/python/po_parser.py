#!/usr/bin/env python2.7
#coding: utf8
from __future__ import print_function

from argparse import ArgumentParser
from categories_converter import CategoriesTxt
from collections import defaultdict
from find_untranslated_strings import StringsTxt
from os import listdir
from os.path import isfile, join


TRANSFORMATION_TABLE = {
    "zh_CN": "zh-Hans",
    "zh_TW": "zh-Hant",
    "no_NO": "nb",
    "en_GB": "en-GB"
}


#msgid
#msgstr
class PoParser:
    def __init__(self):
        args = self.parse_args()
        self.folder_path = args.folder
        self.all_po_files = self.find_all_po_files()

        if (args.strings_txt):
            self.dest_file = StringsTxt(args.strings_txt)
        elif (args.categories_txt):
            self.dest_file = CategoriesTxt(args.categories_txt)
        else:
            raise RuntimeError("You must specify either -s or -c")


    def find_all_po_files(self):
        return [
            f for f in listdir(self.folder_path)
            if isfile(join(self.folder_path, f)) and f.endswith(".po")
        ]


    def parse_files(self):
        for po_file in self.all_po_files:
            self._parse_one_file(
                join(self.folder_path, po_file),
                self.lang_from_filename(po_file)
            )


    def lang_from_filename(self, filename):
        # file names are in this format: strings_ru_RU.po
        lang = filename[len("strings_"):-len(".po")]
        if lang in TRANSFORMATION_TABLE:
            return TRANSFORMATION_TABLE[lang]
        return lang[:2]


    def _parse_one_file(self, filepath, lang):
        self.translations = defaultdict(str)
        current_key = None
        string_started = False
        with open(filepath) as infile:
            for line in infile:
                if line.startswith("msgid"):
                    current_key = self.clean_line(line,"msgid")
                elif line.startswith("msgstr"):
                    if not current_key:
                        continue
                    translation = self.clean_line(line, "msgstr")
                    if not translation:
                        print("No translation for key {} in file {}".format(current_key, filepath))
                        continue
                    self.dest_file.add_translation(
                        translation,
                        key="[{}]".format(current_key),
                        lang=lang
                    )
                    string_started = True

                elif not line or line.startswith("#"):
                    string_started = False
                    current_key = None

                else:
                    if not string_started:
                        continue
                    self.dest_file.append_to_translation(current_key, lang, self.clean_line(line))


    def clean_line(self, line, prefix=""):
        return line[len(prefix):].strip().strip('"')


    def parse_args(self):
        parser = ArgumentParser(
            description="""
            A script for parsing strings in the PO format, which is used by our
            translation partners. The script can pull strings from .po files into
            categories.txt or strings.txt.
            """
        )

        parser.add_argument(
            "-f", "--folder",
            dest="folder", required=True,
            help="""Path to the folder where the PO files are. Required."""
        )

        parser.add_argument(
            "-s", "--strings",
            dest="strings_txt",
            help="""The path to the strings.txt file. The strings from the po
            files will be added to that strings.txt file."""
        )

        parser.add_argument(
            "-c", "--categories",
            dest="categories_txt",
            help="""The path to the categories.txt file. The strings from the po
            files will be added to that categories.txt file."""
        )
        return parser.parse_args()


def main():
    parser = PoParser()
    parser.parse_files()
    parser.dest_file.write_formatted()


if __name__ == "__main__":
    main()
