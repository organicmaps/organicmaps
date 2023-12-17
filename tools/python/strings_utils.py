#!/usr/bin/env python3

from argparse import ArgumentParser
from collections import namedtuple, defaultdict
from itertools import combinations
from os.path import join, dirname, abspath, isabs
import re
from sys import argv


class StringsTxt:

    STRINGS_TXT_PATH = "data/strings/strings.txt"
    TYPES_STRINGS_TXT_PATH = "data/strings/types_strings.txt"

    SECTION = re.compile(r"\[\[\w+.*\]\]")
    DEFINITION = re.compile(r"\[\w+.*\]")
    LANG_KEY = re.compile(r"^[a-z]{2}(-[a-zA-Z]{2,4})?(:[a-z]+)?$")
    TRANSLATION = re.compile(r"^\s*\S+\s*=\s*\S+.*$", re.S | re.MULTILINE)
    MANY_DOTS = re.compile(r"\.{4,}")
    SPACE_PUNCTUATION = re.compile(r"\s[.,?!:;]")
    PLACEHOLDERS = re.compile(r"(%\d*\$@|%[@dqus]|\^)")

    PLURAL_KEYS = frozenset(("zero", "one", "two", "few", "many", "other"))
    SIMILARITY_THRESHOLD = 20.0  # %

    TransAndKey = namedtuple("TransAndKey", "translation, key")

    def __init__(self, strings_path):
        self.strings_path = strings_path

        # dict<key, dict<lang, translation>>
        self.translations = defaultdict(lambda: defaultdict(str))
        self.translations_by_language = defaultdict(
            dict)  # dict<lang, dict<key, translation>>
        self.comments_tags_refs = defaultdict(
            dict)  # dict<key, dict<key, value>>
        self.all_langs = set()  # including plural keys, e.g. en:few
        self.langs = set()  # without plural keys
        self.duplicates = {}  # dict<lang, TransAndKey>
        self.keys_in_order = []
        self.validation_errors = False

        self._read_file()

    def process_file(self):
        self._resolve_references()
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
        lang, plural_key = self._parse_lang(lang)
        self.langs.add(lang)

    def append_to_translation(self, key, lang, tail):
        self.translations[key][lang] = self.translations[key][lang] + tail

    def _read_file(self):
        with open(self.strings_path, encoding='utf-8') as strings:
            for line in strings:
                line = line.strip()
                if not line:
                    continue

                if self.SECTION.match(line):
                    self.keys_in_order.append(line)
                    continue

                if self.DEFINITION.match(line):
                    if line in self.translations:
                        self._print_validation_issue(
                            "Duplicate definition: {0}".format(line))
                    self.translations[line] = {}
                    current_key = line
                    if current_key not in self.keys_in_order:
                        self.keys_in_order.append(current_key)
                    continue

                if self.TRANSLATION.match(line):
                    lang, tran = self._parse_lang_and_translation(line)

                    if lang == "comment" or lang == "tags" or lang == "ref":
                        self.comments_tags_refs[current_key][lang] = tran
                        continue

                    self.translations[current_key][lang] = tran

                    self.all_langs.add(lang)
                    lang, plural_key = self._parse_lang(lang)
                    self.langs.add(lang)

                else:
                    self._print_validation_issue(
                        "Couldn't parse line: {0}".format(line))

    def print_languages_stats(self, langs=None):
        self._print_header("Languages statistics")
        print("All languages in the file ({0} total):\n{1}\n".format(
            len(self.langs), ",".join(sorted(self.langs)))
        )
        print("Regional languages:\n{0}\n".format(
            ",".join([lang for lang in sorted(self.langs) if len(lang) > 2]))
        )
        print("Languages using plurals:\n{0}\n".format(
            ",".join([lang for lang in sorted(self.all_langs) if lang.find(":") > -1]))
        )

        self.print_invalid_languages()

        print_plurals = True
        if not langs:
            print_plurals = False
            langs = self.langs

        langs_stats = []
        plurals_stats = defaultdict(dict)  # dict<lang, dict<plural, int>>
        for lang in langs:
            lang_defs = set()
            if lang in self.translations_by_language:
                lang_defs = set(self.translations_by_language[lang].keys())
                plurals_stats[lang][lang] = len(lang_defs)
            for plural_key in self.PLURAL_KEYS:
                lang_plural = "{0}:{1}".format(lang, plural_key)
                if lang_plural in self.translations_by_language:
                    plural_defs = set(
                        self.translations_by_language[lang_plural].keys())
                    plurals_stats[lang][lang_plural] = len(plural_defs)
                    lang_defs = lang_defs.union(plural_defs)
            langs_stats.append((lang, len(lang_defs)))

        print("\nNumber of translations out of total:\n")

        langs_stats.sort(key=lambda x: x[1], reverse=True)

        n_trans = len(self.translations)
        for lang, lang_stat in langs_stats:
            print("{0:7} : {1} / {2} ({3}%)".format(
                lang, lang_stat, n_trans, round(100 * lang_stat / n_trans)
            ))
            if print_plurals and not (len(plurals_stats[lang]) == 1 and lang in plurals_stats[lang]):
                for lang_plural, plural_stat in plurals_stats[lang].items():
                    print("    {0:13} : {1}".format(lang_plural, plural_stat))

    def print_invalid_languages(self):
        invalid_langs = []
        invalid_plurals = []
        for lang in self.all_langs:
            if not self.LANG_KEY.match(lang):
                invalid_langs.append(lang)
            lang_key, plural_key = self._parse_lang(lang)
            if plural_key and plural_key not in self.PLURAL_KEYS:
                invalid_plurals.append(lang)

        if invalid_langs:
            self._print_validation_issue("Invalid languages: {0}".format(
                ",".join(sorted(invalid_langs))
            ))

        if invalid_plurals:
            self._print_validation_issue("Invalid plurals: {0}".format(
                ",".join(sorted(invalid_plurals))
            ))

    def print_definitions_stats(self, langs=None):
        self._print_header("Definitions stats")
        print("Number of translations out of total:\n")
        if not langs:
            langs = self.langs
        def_stats = []
        for definition in self.translations.keys():
            def_langs = set()
            for def_lang in self.translations[definition].keys():
                def_lang, plural_key = self._parse_lang(def_lang)
                if def_lang in langs:
                    def_langs.add(def_lang)
            def_stats.append((definition, len(def_langs)))
        def_stats.sort(key=lambda x: x[1], reverse=True)

        n_langs = len(langs)
        for definition, n_trans in def_stats:
            print("{0}\t{1} / {2} ({3}%)".format(
                definition, n_trans, n_langs, round(100 * n_trans / n_langs)
            ))

    def print_duplicates(self, langs=None):
        self._print_header("Duplicate translations")
        print("Same translations used in several definitions:")
        langs = self._expand_plurals(langs) if langs else self.all_langs
        dups = list(self.duplicates.items())
        dups.sort(key=lambda x: x[0])
        for lang, trans_and_keys in dups:
            if lang not in langs:
                continue
            print("\nLanguage: {0}".format(lang))
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
            print("\t{0}: {1}".format(",".join(keys), last_one))

    def _expand_plurals(self, langs):
        expanded_langs = set()
        for lang_plural in self.all_langs:
            lang, plural_key = self._parse_lang(lang_plural)
            if lang in langs:
                expanded_langs.add(lang_plural)
        return expanded_langs

    def _parse_lang(self, lang):
        plural_key = None
        sep_pos = lang.find(":")
        if sep_pos > -1:
            lang, plural_key = lang.split(":")
        return lang, plural_key

    def _parse_lang_and_translation(self, line):
        lang, trans = line.split("=", 1)
        if self.MANY_DOTS.search(trans):
            self._print_validation_issue(
                "4 or more dots in the string: {0}".format(line), warning=True)
        return (lang.strip(), trans.strip())

    def _resolve_references(self):
        resolved = set()
        for definition in list(self.comments_tags_refs.keys()):
            visited = set()
            self._resolve_ref(definition, visited, resolved)

    def _resolve_ref(self, definition, visited, resolved):
        visited.add(definition)
        ref = self.comments_tags_refs[definition].get("ref")
        if definition not in resolved and ref:
            ref = "[{0}]".format(ref)
            if ref not in self.translations:
                self._print_validation_issue("Couldn't find reference: {0}".format(self.comments_tags_refs[definition]["ref"]))
                resolved.add(definition)
                return
            if ref in visited:
                self._print_validation_issue("Circular reference: {0} in {1}".format(self.comments_tags_refs[definition]["ref"], visited))
            else:
                # resolve nested refs recursively
                self._resolve_ref(ref, visited, resolved)
            for lang, trans in self.translations[ref].items():
                if lang not in self.translations[definition]:
                    self.translations[definition][lang] = trans
            resolved.add(definition)

    def _populate_translations_by_langs(self):
        for lang in self.all_langs:
            trans_for_lang = {}
            for key, tran in self.translations.items():  # (tran = dict<lang, translation>)
                if lang not in tran:
                    continue
                trans_for_lang[key] = tran[lang]
            self.translations_by_language[lang] = trans_for_lang

    def _find_duplicates(self):
        for lang, tran in self.translations_by_language.items():
            trans_for_lang = [self.TransAndKey(
                x[1], x[0]) for x in tran.items()]
            trans_for_lang.sort(key=lambda x: x.translation)
            prev_tran = self.TransAndKey("", "")
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

        self.most_duplicated = sorted(
            most_duplicated.items(), key=lambda x: x[1], reverse=True)

    def print_most_duplicated(self):
        self._print_header("Most duplicated")
        print("Definitions with the most translations shared with other definitions:\n")
        for pair in self.most_duplicated:
            print("{}\t{}".format(pair[0], pair[1]))

    def print_missing_translations(self, langs=None):
        self._print_header("Untranslated definitions")
        if not langs:
            langs = sorted(self.langs)
        all_translation_keys = set(self.translations.keys())
        for lang in langs:
            keys_for_lang = set(self.translations_by_language[lang].keys())
            missing_keys = all_translation_keys - keys_for_lang
            for plural_key in self.PLURAL_KEYS:
                lang_plural = "{0}:{1}".format(lang, plural_key)
                if lang_plural in self.translations_by_language:
                    missing_keys -= set(
                        self.translations_by_language[lang_plural].keys())
            missing_keys = sorted(missing_keys)
            print("Language: {0} ({1} missing)\n\t{2}\n".format(
                lang, len(missing_keys), "\n\t".join(missing_keys)))

    def write_formatted(self, target_file=None, langs=None, keep_resolved=False):
        before_block = ""
        langs = self._expand_plurals(langs) if langs else self.all_langs
        en_langs = []
        other_langs = []
        for lang in langs:
            if lang.startswith("en"):
                en_langs.append(lang)
            else:
                other_langs.append(lang)
        sorted_langs = sorted(en_langs) + sorted(other_langs)

        if target_file is None:
            target_file = self.strings_path
        with open(target_file, "w", encoding='utf-8') as outfile:
            for key in self.keys_in_order:
                # TODO: sort definitions and sections too?
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

                ref_tran = {}
                if key in self.comments_tags_refs:
                    for k, v in self.comments_tags_refs[key].items():
                        outfile.write("    {0} = {1}\n".format(k, v))
                        if not keep_resolved and k == "ref":
                            ref_tran = self.translations.get("[{0}]".format(v))

                self._write_translations_for_langs(outfile, sorted_langs, tran, ref_tran)

    def _write_translations_for_langs(self, outfile, langs, tran, ref_tran):
        for lang in langs:
            # don't output translation if it's duplicated in referenced definition
            if lang in tran and tran[lang] != ref_tran.get(lang):
                outfile.write("    {0} = {1}\n".format(
                    lang, tran[lang].replace("...", "…")
                ))

    def _compare_blocks(self, key_1, key_2):
        block_1 = self.translations[key_1]
        block_2 = self.translations[key_2]

        common_keys = set(block_1.keys()).intersection(set(block_2))

        common_elements = 0
        for key in common_keys:
            if block_1[key] == block_2[key]:
                common_elements += 1

        sim_index = round(100 * 2 * common_elements /
                          (len(block_1) + len(block_2)))
        if sim_index >= self.SIMILARITY_THRESHOLD:
            return [("{} <-> {}".format(key_1, key_2), sim_index)]
        return []

    def _find_most_similar(self):
        search_scope = [x for x in self.most_duplicated if x[1]
                        > len(self.translations[x[0]]) / 10]
        for one, two in combinations(search_scope, 2):
            self.similarity_indices.extend(
                self._compare_blocks(one[0], two[0]))

        self.similarity_indices.sort(key=lambda x: x[1], reverse=True)

    def print_most_similar(self):
        self._print_header("Most similar definitions")
        print("Definitions most similar to other definitions, i.e. with a lot of same translations:\n")
        for index in self.similarity_indices:
            print("{} : {}%".format(index[0], index[1]))

    def _print_header(self, string):
        # print headers in green colour
        print("\n{line} \033[0;32m{str}\033[0m {line}\n".format(
            line="=" * round((70 - len(string)) / 2),
            str=string
        ))

    def _print_validation_issue(self, issue, warning=False):
        if warning:
            # print warnings in yellow colour
            print("\033[0;33mWARNING: {0}\033[0m".format(issue))
            return
        self.validation_errors = True
        # print errors in red colour
        print("\033[0;31mERROR: {0}\033[0m".format(issue))

    def _has_space_before_punctuation(self, lang, string):
        if lang == "fr":  # make exception for French
            return False
        if self.SPACE_PUNCTUATION.search(string):
            return True
        return False

    def print_strings_with_spaces_before_punctuation(self, langs=None):
        self._print_header("Strings with spaces before punctuation")
        langs = self._expand_plurals(langs) if langs else self.all_langs
        for key, lang_and_trans in self.translations.items():
            wrote_key = False
            for lang, translation in lang_and_trans.items():
                if lang in langs:
                    if self._has_space_before_punctuation(lang, translation):
                        if not wrote_key:
                            print("\n{}".format(key))
                            wrote_key = True
                        self._print_validation_issue(
                            "{0} : {1}".format(lang, translation), warning=True)

    def _check_placeholders_in_block(self, block_key, langs):
        wrong_placeholders_strings = []
        en_lang = "en"
        en_trans = self.translations[block_key].get(en_lang)
        if not en_trans:
            for plural_key in sorted(self.PLURAL_KEYS):
                if en_trans:
                    break
                en_lang = "en:{0}".format(plural_key)
                en_trans = self.translations[block_key].get(en_lang)
            if not en_trans:
                self._print_validation_issue(
                    "No English for definition: {}".format(block_key))
                return None, wrong_placeholders_strings

        en_placeholders = sorted(self.PLACEHOLDERS.findall(en_trans))

        for lang, translation in self.translations[block_key].items():
            found = sorted(self.PLACEHOLDERS.findall(translation))
            if not en_placeholders == found:  # must be sorted
                wrong_placeholders_strings.append(
                    "{} = {}".format(lang, translation))

        return en_lang, wrong_placeholders_strings

    def print_strings_with_wrong_placeholders(self, langs=None):
        self._print_header("Strings with a wrong number of placeholders")
        langs = self._expand_plurals(langs) if langs else self.all_langs
        for key, lang_and_trans in self.translations.items():
            en_lang, wrong_placeholders = self._check_placeholders_in_block(
                key, langs)
            if not wrong_placeholders:
                continue

            print("\n{0}".format(key))
            print("{0} = {1}".format(en_lang, lang_and_trans[en_lang]))
            for wp in wrong_placeholders:
                self._print_validation_issue(wp)

    def validate(self, langs=None):
        self._print_header("Validating the file...")
        if self.validation_errors:
            self._print_validation_issue(
                "There were errors reading the file, check the output above")
        self._print_header("Invalid languages")
        self.print_invalid_languages()
        self.print_strings_with_spaces_before_punctuation(langs=args.langs)
        self.print_strings_with_wrong_placeholders(langs=args.langs)
        return not self.validation_errors


def find_project_root():
    my_path = abspath(__file__)
    tools_index = my_path.rfind("/tools/python")
    project_root = my_path[:tools_index]
    return project_root


def get_args():
    parser = ArgumentParser(
        description="""
        Validates and formats translation files (strings.txt, types_strings.txt),
        prints file's statistics, finds duplicate and missing translations, etc."""
    )

    parser.add_argument(
        "input",
        nargs="?", default=None,
        help="input file path, defaults to <organicmaps>/data/strings/strings.txt"
    )

    parser.add_argument(
        "-t", "--types-strings",
        action="store_true",
        help="use <organicmaps>/data/strings/types_strings.txt as input file by default"
    )

    parser.add_argument(
        "-o", "--output",
        default=None, nargs="?", const=True,
        help="""path to write formatted output file to with languages
        sorted in alphabetic order except English translations going first
        (overwrites the input file by default)"""
    )

    parser.add_argument(
        "--keep-resolved-references",
        dest="keep_resolved",
        action="store_true",
        help="""keep resolved translation references when writing output file;
        used with --output only"""
    )

    parser.add_argument(
        "-l", "--languages",
        dest="langs", default=None,
        help="a comma-separated list of languages to limit output to, if applicable"
    )

    parser.add_argument(
        "-pl", "--print-languages",
        dest="print_langs",
        action="store_true",
        help="print languages statistics"
    )

    parser.add_argument(
        "-pf", "--print-definitions",
        dest="print_defs",
        action="store_true",
        help="print definitions stattistics"
    )

    parser.add_argument(
        "-pd", "--print-duplicates",
        dest="print_dups",
        action="store_true",
        help="print same translations used in several definitions"
    )

    parser.add_argument(
        "-po", "--print-most-duplicated",
        dest="print_mdups",
        action="store_true",
        help="""print definitions with the most translations shared
        with other definitions"""
    )

    parser.add_argument(
        "-ps", "--print-similar",
        dest="print_similar",
        action="store_true",
        help="""print definitions most similar to other definitions,
        i.e. with a lot of same translations"""
    )

    parser.add_argument(
        "-pm", "--missing-translations",
        dest="print_missing",
        action="store_true",
        help="print untranslated definitions"
    )

    parser.add_argument(
        "-v", "--validate",
        dest="validate",
        action="store_true",
        help="""validate file format, placeholders usage, whitespace
        before punctuation, etc; exit with error if not valid"""
    )

    return parser.parse_args()


if __name__ == "__main__":
    import sys

    args = get_args()

    if not args.input:
        args.input = StringsTxt.TYPES_STRINGS_TXT_PATH if args.types_strings else StringsTxt.STRINGS_TXT_PATH
        args.input = "{0}/{1}".format(find_project_root(), args.input)
    args.input = abspath(args.input)
    print("Input file: {0}\n".format(args.input))

    strings = StringsTxt(args.input)
    strings.process_file()

    if args.langs:
        args.langs = args.langs.split(",")
        print("Limit output to languages:\n{0}\n".format(",".join(args.langs)))

    if args.print_langs:
        strings.print_languages_stats(langs=args.langs)

    if args.print_defs:
        strings.print_definitions_stats(langs=args.langs)

    if args.print_dups:
        strings.print_duplicates(langs=args.langs)

    if args.print_mdups:
        strings.print_most_duplicated()

    if args.print_similar:
        strings.print_most_similar()

    if args.print_missing:
        strings.print_missing_translations(langs=args.langs)

    if args.validate:
        if not strings.validate(langs=args.langs):
            # print in red color
            print("\n\033[0;31mThe file is not valid, terminating\033[0m")
            sys.exit(1)

    if args.output:
        if args.output == True:
            args.output = args.input
        else:
            args.output = abspath(args.output)
        print("\nWriting formatted output file: {0}\n".format(args.output))
        strings.write_formatted(target_file=args.output, langs=args.langs, keep_resolved=args.keep_resolved)
