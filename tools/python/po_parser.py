#!/usr/bin/env python2.7

from __future__ import print_function

from collections import defaultdict
from argparse import ArgumentParser
from os import listdir
from os.path import isfile, join
from find_untranslated_strings import StringsTxt


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
        self.strings_txt = StringsTxt(args.strings_txt)


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
                    self.strings_txt.add_translation(
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
                    self.strings_txt.append_to_translation(current_key, lang, self.clean_line(line))


    def clean_line(self, line, prefix=""):
        return line[len(prefix):].strip().strip('"')


    def parse_args(self):
        parser = ArgumentParser(
            description="""
            A script for parsing stirngs in the PO format, which is used by our
            translation partners.
            """
        )

        parser.add_argument(
            "-f", "--folder",
            dest="folder", required=True,
            help="""Path to the folder where the PO files are. Required."""
        )

        parser.add_argument(
            "-s", "--strings-txt",
            dest="strings_txt", required=True,
            help="""The path to the strings.txt file. The strings from the po
            files will be added to that strings.txt file."""
        )

        return parser.parse_args()


def main():
    parser = PoParser()
    parser.parse_files()
    parser.strings_txt.write_formatted()

if __name__ == "__main__":
    main()