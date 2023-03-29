# Translations

Adding and updating translations is easy!
1. Change the translation file you want, e.g. [strings.txt](../data/strings/strings.txt) ([raw text version](https://raw.githubusercontent.com/organicmaps/organicmaps/master/data/strings/strings.txt))
2. Commit your changes
3. Send a pull request!

Please prepend `[strings]` to your commit message and add [Developers Certificate of Origin](CONTRIBUTING.md#legal-requirements) to it.

Then run a `tools/unix/generate_localizations.sh` script and add the changes as a separate `[strings] Regenerated` commit.
But if you can't run it - don't worry, its not mandatory!

## Requirements

To run the `tools/unix/generate_localizations.sh` script, it is necessary to have installed `ruby`.

## Translation files

Main:
- Application UI strings: [`data/strings/strings.txt`](../data/strings/strings.txt)
- A few iOS specific strings: [`iphone/plist.txt`](../iphone/plist.txt)
- Names of map features/types: [`data/strings/types_strings.txt`](../data/strings/types_strings.txt)
- Search keywords/aliases/synonyms for map features: [`data/categories.txt`](../data/categories.txt)

Additional:
- Text-to-speech strings for navigation: [`data/strings/sound.txt`](../data/strings/sound.txt)

- Android stores description: [`android/src/google/play/listings/`](../android/src/google/play/listings/)
- Apple AppStore description: [`iphone/metadata/`](../iphone/metadata/)

- Popular brands of map features: [`data/strings/brands_strings.txt`](../data/strings/brands_strings.txt)
- Search keywords for popular brands: [`data/categories_brands.txt`](../data/categories_brands.txt)
- Search keywords for cuisine types: [`data/categories_cuisines.txt`](../data/categories_cuisines.txt)

- Country / map region names: [`data/countries_names.txt`](../data/countries_names.txt)

- [other strings](STRUCTURE.md#strings-and-translations) files

Language codes used are from [ISO 639-1 standard](https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes).
If a string is not translated into a particular language then it falls back to English or a "parent" language (e.g. `es-MX` falls back to `es`).

## Tools

To find strings without translations substitute `ar` with your language code and run the following script:
```
tools/python/strings_utils.py -l ar -pm
```
By default, it searches `strings.txt`, to check `types_strings.txt` add a `-t` option.
There are many more other options, e.g. print various translation statistics, validate and re-format translation files.
Check `tools/python/strings_utils.py -h` to see all of them.

In some cases automatically translated strings are better than no translation at all.
There is a script to automate given string's translation into multiple languages.
Please [install Translate Shell](https://www.soimort.org/translate-shell/#installation) first to be able to run it.

```bash
# This generates translations in categories.txt format
tools/unix/translate_categories.sh "Route"
# Translations in strings.txt format
DELIM=" = " tools/unix/translate_categories.sh "Route"
```

## Technical details

Most of the translation files (strings, types_strings...) are in Twine file format ([syntax reference](https://github.com/organicmaps/twine/blob/organicmaps/README.md)).
OM uses a custom version of the [Twine](https://github.com/organicmaps/twine)
tool (resides in `tools/twine/` submodule) to generate platform-native (Android, iOS)
localization files from a single translation file.

The `tools/unix/generate_localizations.sh` script launches this conversion
(and installs Twine beforehand if necessary).

Search keywords translation files use a custom format described in the beginning of `data/categories.txt`.

A `tools/python/clean_strings_txt.py` script is used to sync `strings.txt` with real UI strings usage as found in the codebase.

There are preliminary plans to refactor translations workflow and migrate to Weblate.
