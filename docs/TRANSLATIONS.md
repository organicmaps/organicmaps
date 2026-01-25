# Translations

## Contribute via Weblate

[Weblate](https://hosted.weblate.org/projects/organicmaps/) translations of the app and map types are integrated manually on a regular basis.

## Contribute translations directly

Adding and updating translations is easy!
1. Change the translation file you want, e.g. [strings.txt](../data/strings/strings.txt) ([raw text version](https://raw.githubusercontent.com/organicmaps/organicmaps/master/data/strings/strings.txt))
2. If you are developer, continue below, otherwise send us the translated file for manual integration.
3. Commit your string changes with the title `[strings] {description of changes}`
4. (Optional) run the `tools/unix/generate_localizations.sh` script
5. (Optional) Commit the updated files with the title `[strings] Regenerated`
6. Send a pull request!

Please make sure to add a [Developers Certificate of Origin](CONTRIBUTING.md#legal-requirements) to your commit descriptions.

## Requirements

To run the `tools/unix/generate_localizations.sh` script, it is necessary to have installed Python 3.10+.

## Components

- App translations [strings.txt](../data/strings/strings.txt)
- Map types [types_strings.txt](../data/strings/types_strings.txt)
- TTS voice directions [sound.txt](../data/strings/sound.txt)
- Country names [countries-strings](../data/countries-strings/)
- Search synonyms [categories.txt](../data/categories.txt) and [categories_cuisines.txt](../data/categories_cuisines.txt)
- Website translations: see [website/README.md](https://github.com/organicmaps/website/blob/master/README.md#translations)
- App Store metadata (iOS): [metadata/en-US](../iphone/metadata/en-US)
- Google Play metadata (Android): [play/listings/en-US](../android/app/src/fdroid/play/listings/)
