#!/usr/bin/env python
# coding: utf-8
from __future__ import print_function
from collections import namedtuple, defaultdict
from itertools import combinations
from os.path import join, dirname
import re
from sys import argv

TransAndKey = namedtuple("TransAndKey", "translation, key")

TRANSLATION = re.compile(r"(.*)\s*=\s*.*$", re.S | re.MULTILINE)
MANY_DOTS = re.compile(r"\.{4,}")
SPACE_PUNCTUATION = re.compile(r"\s[.,?!:;]")
PLACEHOLDERS = re.compile(r"(%\d*\$@|%[@dqus]|\^)")

ITUNES_LANGS = ["en", "en-GB", "ru", "ar", "cs", "da", "nl", "fi", "fr", "de", "hu", "id", "it", "ja", "ko", "nb", "pl", "pt", "pt-BR", "ro", "sl", "es", "sv", "th", "tr", "uk", "vi", "zh-Hans", "zh-Hant"]

SIMILARITY_THRESHOLD = 20.0 #%


class StringsTxt:

    def __init__(self, strings_path=None):
        if not strings_path:
            self.strings_path = join(dirname(argv[0]), "..", "..", "strings.txt")
        else:
            self.strings_path = strings_path

        self.translations = defaultdict(lambda: defaultdict(str)) # dict<key, dict<lang, translation>>
        self.translations_by_language = defaultdict(dict) # dict<lang, dict<key, translation>>
        self.comments_and_tags = defaultdict(dict)
        self.with_english = []
        self.all_langs = set()
        self.duplicates = {} # dict<lang, TransAndKey>
        self.keys_in_order = []
        self._read_file()


    def process_file(self):
        self._populate_translations_by_langs()
        self._find_duplicates()
        self.most_duplicated = []
        self._find_most_duplicated()
        self.similarity_indices = []
        self._find_most_similar()


    def add_translation(self, translation, key, lang):
        if key not in self.keys_in_order:
            self.keys_in_order.append(key)
        self.translations[key][lang] = translation
        self.all_langs.add(lang)


    def append_to_translation(self, key, lang, tail):
        self.translations[key][lang] = self.translations[key][lang] + tail


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
                    # if line in self.translations:
                    #     print("Duplicate key {}".format(line))
                    #     continue
                    self.translations[line] = {}
                    current_key = line
                    if current_key not in self.keys_in_order:
                        self.keys_in_order.append(current_key)

                if TRANSLATION.match(line):
                    lang, tran = self._parse_lang_and_translation(line)

                    if lang == "comment" or lang == "tags":
                        self.comments_and_tags[current_key][lang] = tran
                        continue

                    self.translations[current_key][lang] = tran

                    self.all_langs.add(lang)
                    if line.startswith("en = "):
                        self.with_english.append(current_key)
                    continue


    def print_statistics(self):
        stats = map(lambda x: (x, len(self.translations[x])), self.translations.keys())
        stats.sort(key=lambda x: x[1], reverse=True)

        for k, v in stats:
            print("{0}\t{1}".format(k, v))


    def print_duplicates(self):
        print(self._header("Duplicates:"))
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


    def _parse_lang_and_translation(self, line):
        ret = tuple(map(self._process_string, line.split("=", 1)))
        if len(ret) < 2:
            print("ERROR: Couldn't parse the line: {0}".format(line))
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

    def _find_most_duplicated(self):
        most_duplicated = defaultdict(int)
        for trans_and_keys in self.duplicates.values():
            for trans_and_key in trans_and_keys:
                most_duplicated[trans_and_key.key] += 1

        self.most_duplicated = sorted(most_duplicated.items(), key=lambda x: x[1], reverse=True)


    def print_most_duplicated(self):
        print(self._header("Most duplicated"))
        for pair in self.most_duplicated:
            print("{}\t{}".format(pair[0], pair[1]))


    def print_missing_itunes_langs(self):
        print(self._header("Missing translations for iTunes languages:"))

        all_translation_keys = set(self.translations.keys())
        for lang in ITUNES_LANGS:
            keys_for_lang = set(self.translations_by_language[lang].keys())
            missing_keys = sorted(list(all_translation_keys - keys_for_lang))
            print("{0}:\n{1}\n".format(lang, "\n".join(missing_keys)))


    def write_formatted(self, target_file=None, languages=None):
        before_block = ""
        if target_file is None:
            target_file = self.strings_path
        non_itunes_langs = sorted(list(self.all_langs - set(ITUNES_LANGS)))
        with open(target_file, "w") as outfile:
            for key in self.keys_in_order:
                if not key:
                    continue
                if key in self.translations:
                    tran = self.translations[key]
                else:
                    if key.startswith("[["):
                        outfile.write("{0}{1}\n".format(before_block, key))
                        before_block = "\n"
                    continue

                outfile.write("{0}  {1}\n".format(before_block, key))
                before_block = "\n"

                if key in self.comments_and_tags:
                    for k, v in self.comments_and_tags[key].items():
                        outfile.write("    {0} = {1}\n".format(k, v))
                self._write_translations_for_langs(ITUNES_LANGS, tran, outfile, only_langs=languages)
                self._write_translations_for_langs(non_itunes_langs, tran, outfile, only_langs=languages)


    def _write_translations_for_langs(self, langs, tran, outfile, only_langs=None):
        langs_to_write = []

        if only_langs:
            for lang in only_langs:
                if lang in langs:
                    langs_to_write.append(lang)
        else:
            langs_to_write = langs

        for lang in langs_to_write:
            if lang in tran:
                outfile.write("    {0} = {1}\n".format(lang, tran[lang]))


    def _compare_blocks(self, key_1, key_2):
        block_1 = self.translations[key_1]
        block_2 = self.translations[key_2]

        common_keys = set(block_1.keys()).intersection(set(block_2))

        common_elements = 0
        for key in common_keys:
            if block_1[key] == block_2[key]:
                common_elements += 1

        return filter(lambda x: x[1] > SIMILARITY_THRESHOLD, [
            (self._similarity_string(key_1, key_2), self._similarity_index(len(block_1), common_elements)),
            (self._similarity_string(key_2, key_1), self._similarity_index(len(block_2), common_elements))
        ])


    def _similarity_string(self, key_1, key_2):
        return "{} -> {}".format(key_1, key_2)


    def _similarity_index(self, total_number, number_from_other):
        return 100.0 * number_from_other / total_number


    def _find_most_similar(self):
        search_scope = filter(lambda x : x[1] > len(self.translations[x[0]]) / 10, self.most_duplicated)
        for one, two in combinations(search_scope, 2):
            self.similarity_indices.extend(self._compare_blocks(one[0], two[0]))

        self.similarity_indices.sort(key=lambda x: x[1], reverse=True)


    def print_most_similar(self):
        print(self._header("Most similar blocks"))
        for index in self.similarity_indices:
            print("{} : {}".format(index[0], index[1]))


    def _header(self, string):
        return "\n\n{line}\n{string}\n{line}\n".format(
            line="=" * 80,
            string=string
        )


    def _has_space_before_punctuation(self, lang, string):
        if lang == "fr":
            return False
        if SPACE_PUNCTUATION.search(string):
            return True
        return False


    def print_strings_with_spaces_before_punctuation(self):
        print(self._header("Strings with spaces before punctuation:"))
        for key, lang_and_trans in self.translations.items():
            wrote_key = False
            for lang, translation in lang_and_trans.items():
                if self._has_space_before_punctuation(lang, translation):
                    if not wrote_key:
                        print("\n{}".format(key))
                        wrote_key = True
                    print("{} : {}".format(lang, translation))


    def _check_placeholders_in_block(self, block_key):
        wrong_placeholders_strings = []
        key = self.translations[block_key].get("en")
        if not key:
            print("No english for key: {}".format(block_key))
            print("Existing keys are: {}".format(",".join(self.translations[block_key].keys())))
            raise KeyError

        en_placeholders = sorted(PLACEHOLDERS.findall(key))

        
        for lang, translation in self.translations[block_key].items():
            if lang == "en":
                continue
            found = sorted(PLACEHOLDERS.findall(translation))
            if not en_placeholders == found: #must be sorted
                wrong_placeholders_strings.append("{} : {}".format(lang, translation))

        return wrong_placeholders_strings


    def print_strings_with_wrong_paceholders(self):
        print(self._header("Strings with a wrong number of placeholders:"))
        for key, lang_and_trans in self.translations.items():
            wrong_placeholders = self._check_placeholders_in_block(key)
            if not wrong_placeholders:
                continue

            print("\n{0}".format(key))
            print("English: {0}".format(lang_and_trans["en"]))
            for string in wrong_placeholders:
                print(string)


if __name__ == "__main__":
    strings = StringsTxt()
    strings.process_file()
    strings.print_statistics()
    strings.print_duplicates()
    strings.print_most_duplicated()
    strings.print_most_similar()
    strings.print_missing_itunes_langs()
    strings.write_formatted()
    strings.print_strings_with_spaces_before_punctuation()
    strings.print_strings_with_wrong_paceholders()
