#!/usr/bin/env python
# coding: utf-8
from __future__ import print_function
from collections import namedtuple, defaultdict
from os.path import join, dirname
import re
from sys import argv

TransAndKey = namedtuple("TransAndKey", "translation, key")

TRANSLATION = re.compile(r"([a-z]{2}|zh-Han[st])\s*=\s*.*$", re.S | re.MULTILINE)
MANY_DOTS = re.compile(r"\.{4,}")

ITUNES_LANGS = ["en", "ru", "ar", "cs", "da", "nl", "fi", "fr", "de", "hu", "id", "it", "ja", "ko", "nb", "pl", "pt", "ro", "sl", "es", "sv", "th", "tr", "uk", "vi", "zh-Hans", "zh-Hant"]


class StringsTxt:

    def __init__(self):
        self.strings_path = join(dirname(argv[0]), "../..", "strings.txt")
        self.translations = defaultdict(dict) # dict<key, dict<lang, translation>>
        self.translations_by_language = defaultdict(dict) # dict<lang, dict<key, translation>>
        self.comments_and_tags = defaultdict(dict)
        self.with_english = []
        self.all_langs = set()
        self.duplicates = {} # dict<lang, TransAndKey>
        self.keys_in_order = []
        self._read_file()
        self._populate_translations_by_langs()
        self._find_duplicates()


    def _read_file(self):
        with open(self.strings_path) as strings:
            for line in strings:
                line = line.strip()
                if not line:
                    continue
                if line.startswith("[["):
                    self.keys_in_order.append(line)
                    continue
                if line.startswith("["):
                    if line in self.translations:
                        print("Duplicate key {}".format(line))
                        continue
                    self.translations[line] = {}
                    current_key = line
                    self.keys_in_order.append(current_key)

                if TRANSLATION.match(line):
                    lang, tran = self._lang_and_translation(line)
                    self.translations[current_key][lang] = tran

                    self.all_langs.add(lang)
                    if line.startswith("en = "):
                        self.with_english.append(current_key)
                    continue

                if line.startswith("comment") or line.startswith("tags"):
                    lang, value = self._lang_and_translation(line)
                    self.comments_and_tags[current_key][lang] = value
                    continue


    def print_statistics(self):
        stats = map(lambda x: (x, len(self.translations[x])), self.translations.keys())
        stats.sort(key=lambda x: x[1], reverse=True)

        for k, v in stats:
            print("{0}\t{1}".format(k, v))


    def print_duplicates(self):
        print("\n\n========================================\n\nDuplicates: ")
        for lang, trans_and_keys in self.duplicates.items():
            print("{0}\n {1}\n".format("=" * (len(lang) + 2), lang))
            last_one = ""
            keys = []
            for tr in trans_and_keys:
                if last_one != tr.translation:
                    self._print_keys_for_duplicates(keys, last_one)
                    keys = []
                last_one = tr.translation
                keys.append(tr.key)
            self._print_keys_for_duplicates(keys, last_one)


    def _print_keys_for_duplicates(self, keys, last_one):
        if last_one:
            print("{0}: {1}\n".format(", ".join(keys), last_one))


    def _process_string(self, string):
        if MANY_DOTS.search(string):
            print("WARNING: 4 or more dots in the string: {0}".format(string))
        return str.strip(string).replace("...", "…")


    def _lang_and_translation(self, line):
        ret = tuple(map(self._process_string, line.split("=")))
        assert len(ret) == 2
        return ret


    def _populate_translations_by_langs(self):
        for lang in self.all_langs:
            trans_for_lang = {}
            for key, tran in self.translations.items(): # (tran = dict<lang, translation>)
                if lang not in tran:
                    continue
                trans_for_lang[key] = tran[lang]
            self.translations_by_language[lang] = trans_for_lang


    def _find_duplicates(self):
        for lang, tran in self.translations_by_language.items():
            trans_for_lang = map(lambda x: TransAndKey(x[1], x[0]), tran.items())
            trans_for_lang.sort(key=lambda x: x.translation)
            prev_tran = TransAndKey("", "")
            possible_duplicates = set()
            for curr_tran in trans_for_lang:
                if curr_tran.translation == prev_tran.translation:
                    possible_duplicates.add(prev_tran)
                    possible_duplicates.add(curr_tran)
                else:
                    prev_tran = curr_tran

            self.duplicates[lang] = sorted(list(possible_duplicates))


    def print_missing_itunes_langs(self):
        print("\n\n======================================\n\nMissing translations for iTunes languages:")

        all_translation_keys = set(self.translations.keys())
        for lang in ITUNES_LANGS:
            keys_for_lang = set(self.translations_by_language[lang].keys())
            missing_keys = sorted(list(all_translation_keys - keys_for_lang))
            print("{0}:\n{1}\n".format(lang, "\n".join(missing_keys)))


    def write_formatted(self):
        non_itunes_langs = sorted(list(self.all_langs - set(ITUNES_LANGS)))
        with open(self.strings_path, "w") as outfile:
            for key in self.keys_in_order:
                if key in self.translations:
                    tran = self.translations[key]
                else:
                    outfile.write("{0}\n\n".format(key))
                    continue

                outfile.write("  {0}\n".format(key))
                if key in self.comments_and_tags:
                    for k, v in self.comments_and_tags[key].items():
                        outfile.write("    {0} = {1}\n".format(k, v))

                self._write_translations_for_langs(ITUNES_LANGS, tran, outfile)
                self._write_translations_for_langs(non_itunes_langs, tran, outfile)

                outfile.write("\n")


    def _write_translations_for_langs(self, langs, tran, outfile):
        for lang in langs:
            if lang in tran:
                outfile.write("    {0} = {1}\n".format(lang, tran[lang]))


if __name__ == "__main__":
    strings = StringsTxt()
    strings.print_statistics()
    strings.print_duplicates()
    strings.print_missing_itunes_langs()
    strings.write_formatted()
