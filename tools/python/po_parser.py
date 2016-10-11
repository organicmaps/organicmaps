from __future__ import print_function

from collections import defaultdict
from argparse import ArgumentParser
from os import listdir
from os.path import isfile, join


TRANSFORMATION_TABLE = {
    "zh_CN": "zh_Hans",
    "zh_TW": "zh_Hant",
    "no_NO": "no",
    "en_GB": "en_GB"
}


#msgid
#msgstr
class PoParser:
    def __init__(self, folder_path):
        args = self.parse_args()
        self.folder_path = args.folder

        all_po_files = self.find_all_po_files()


    def find_all_po_files(self):
        return [
            f for f in listdir(self.folder_path)
            if isfile(join(self.folder_path, f)) and f.endswith(".po")
        ]


    def parse_files(self):
        for key, tr in self.translations.iteritems():
            print("  [{}]\n    en = {}".format(key, tr))


    def lang_from_filename(self, filename):
        # strings_ru_RU.po
        lang = filename[len("strings_"):-len(".po")]


    def _parse_one_file(self, filepath):
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
                        print("No translation for key {}".format(current_key))
                        continue
                    self.translations[current_key] = translation
                    string_started = True
                elif not line or line.startswith("#"):
                    string_started = False
                    current_key = None
                else:
                    if not string_started:
                        continue
                    self.translations[current_key] = "{}{}".format(self.translations[current_key], self.clean_line(line))




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



def main():
    # parser = PoParser("en.po", "en")
    # for key, tr in parser.translations.iteritems():
    #     print("  [{}]\n    en = {}".format(key, tr))
    pass

if __name__ == "__main__":
    main()