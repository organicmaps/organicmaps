#!/usr/bin/env python2.7
#coding: utf8
from __future__ import print_function

from argparse import ArgumentParser
from collections import defaultdict
from find_untranslated_strings import ITUNES_LANGS


class CategoriesConverter:
    def __init__(self):
        args = self.parse_args()
        self.categories = CategoriesTxt(args.categories)
        self.should_format = args.format
        self.output = args.output


    def process(self):
        if self.should_format:
            self.categories.write_formatted()
        else:
            self.categories.write_as_strings(self.output)


    def parse_args(self):
        parser = ArgumentParser(
            description="""
            A script for converting categories.txt into the strings.txt format
            and back, as well as for autoformatting categories.txt. This is
            useful for interacting with the translation partners.
            """
        )

        parser.add_argument(
            "-c", "--categories",
            required=True,
            dest="categories",
            help="""Path to the categories file to be converted into the strings.txt format."""
        )

        parser.add_argument(
            "-o", "--output",
            dest="output",
            help="""The destination file."""
        )

        parser.add_argument(
            "-f", "--format",
            dest="format", action="store_true", default=False,
            help="""Format the file and exit"""
        )

        return parser.parse_args()


class CategoriesTxt:
    """For now, let's allow comments only at the beginning of the file."""
    def __init__(self, filepath):
        self.translations = defaultdict(lambda: defaultdict(str))
        self.keys_in_order = []
        self.comments = []
        self.filepath = filepath
        self.all_langs = set()
        self.parse_file()


    def parse_file(self):
        current_key = ""
        this_line_is_key = True
        with open(self.filepath) as infile:
            for line in map(str.strip, infile):
                if line.startswith("#"):
                    self.comments.append(line)
                    this_line_is_key = True
                elif not line:
                    this_line_is_key = True
                elif this_line_is_key:
                    self.keys_in_order.append(line)
                    current_key = line
                    this_line_is_key = False
                else:
                    pos = line.index(':')
                    lang = line[:pos]
                    translation = line[pos + 1:]
                    self.translations[current_key][lang] = translation


    def write_as_categories(self, outfile):
        self.write_strings_formatted(outfile, "\n{}\n", "{}:{}\n")


    def write_as_strings(self, filepath):
        with open(filepath, "w") as outfile:
            self.write_strings_formatted(outfile, key_format="\n  [{}]\n", line_format="    {} = {}\n")


    def write_strings_formatted(self, outfile, key_format, line_format):
        for key in self.keys_in_order:
            outfile.write(key_format.format(key.strip("[]")))
            pair = self.translations[key]
            for lang in ITUNES_LANGS:
                if lang in pair:
                    outfile.write(line_format.format(lang, pair[lang]))
            remaining_langs = sorted(list(set(pair.keys()) - set(ITUNES_LANGS)))
            for lang in remaining_langs:
                outfile.write(line_format.format(lang, pair[lang]))


    def add_translation(self, translation, key, lang):
        if key not in self.keys_in_order:
            self.keys_in_order.append(key)
        self.translations[key][lang] = translation


    def append_to_translation(self, translation, key, lang):
        self.translations[key][lang] += translation


    def write_formatted(self):
        with open(self.filepath, "w") as outfile:
            for comment in self.comments:
                outfile.write(comment + "\n")

            self.write_as_categories(outfile)


if __name__ == "__main__":
    c = CategoriesConverter()
    c.process()
