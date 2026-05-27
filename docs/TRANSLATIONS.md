# Translations

## How to contribute translations

Adding and updating translations is easy!
1. Change the translation file you want, e.g. [strings.txt](../data/strings/strings.txt) ([raw text version](https://raw.githubusercontent.com/organicmaps/organicmaps/master/data/strings/strings.txt))
2. If you are a developer, continue below, otherwise send us the translated file for manual integration or create a pull request with your changes via the GitHub web interface.
3. Commit your string changes with the title `[strings] {description of changes}`
4. (Optional) run the `tools/unix/generate_localizations.sh` script
5. (Optional) Commit the updated files with the title `[strings] Regenerated`
6. Send a pull request!

Please make sure to add a [Developers Certificate of Origin](CONTRIBUTING.md#legal-requirements) to your commit descriptions (`git commit -s ...`).

## Requirements

Python 3.10+ is required to run the `tools/unix/generate_localizations.sh` script.

## Components

- App translations [strings.txt](../data/strings/strings.txt)
- Map types [types_strings.txt](../data/strings/types_strings.txt)
- TTS voice directions [sound.txt](../data/strings/sound.txt)
- Country names [countries-strings](../data/countries-strings/)
- Search synonyms [categories.txt](../data/categories.txt) and [categories_cuisines.txt](../data/categories_cuisines.txt)
- Website translations: see [website/README.md](https://github.com/organicmaps/website/blob/master/README.md#translations)
- App Store metadata (iOS): [metadata/en-US](../iphone/metadata/en-US)
- Google Play/FDroid/AppGallery metadata (Android): [play/listings/en-US](../android/app/src/fdroid/play/listings/)

## Notes

Translations via Weblate are not supported anymore for several reasons:
- They were often of poor quality and required manual reviews and follow-up fixes
- They are not compatible with the currently used developer-friendly workflows in the app and on our website
- Weblate is quite expensive for a free, donation-based project with so many translations
- Other issues mentioned in https://github.com/organicmaps/organicmaps/issues/11569

The situation may change if someone implements a Weblate plugin that uses and generates Twine-compatible translation files used by Organic Maps, and supports the Markdown file structure used in Zola- and Hugo-like static site generators.
