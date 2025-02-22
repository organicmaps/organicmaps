# Translations

## For Translators

Translations are maintained using Weblate:

https://hosted.weblate.org/projects/organicmaps/

There is no need to submit translations directly to this repository. Please use Weblate for all translation contributions. Weblate automatically raises pull requests, which are merged by [@organicmaps/mergers](https://github.com/orgs/organicmaps/teams/mergers/members) on a periodic basis.

Weblate components share the same translation memory and vocabulary. The system is configured to automatically propagate translations across components (e.g., from Android to iOS and vice versa). Updating a string in one place should be sufficient.

Please pay close attention to any issues flagged by the automated checks, such as missing string placeholders, inconsistent translations, and other potential errors.

Components:

- [**Android**](https://hosted.weblate.org/projects/organicmaps/android/) – UI strings for the Android app.
- [**iOS**](https://hosted.weblate.org/projects/organicmaps/ios/) – UI strings for the iOS app (excluding plurals).
- [**iOS Plurals**](https://hosted.weblate.org/projects/organicmaps/wip-ios-plurals-please-dont-use/) – UI strings for the iOS app (plurals).
- [**Sound**](https://hosted.weblate.org/projects/organicmaps/sound/) – Text-to-speech strings for voice navigation.
- [**Countries**](https://hosted.weblate.org/projects/organicmaps/sound/) – Country and region names.

## For Developers

Translations are managed by the translation team and do not require direct involvement from developers. Developers should add English base strings using the native tools provided by Android Studio and Xcode.

When implementing a new feature that already exists on another platform, please check for existing keys and reuse the same English base string to maintain consistency across platforms. Manual propagation of strings between platforms is unnecessary, as Weblate handles this automatically.

Regenerating strings is no longer required. Pre-filling languages using machine translation is no longer required too. Feel free to add translations for languages you know in the same PR, or you can always contribute later via Weblate.

Components:

- **Android:**

  - `android/app/src/main/res/values/strings.xml` – Native Android XML format for UI strings.

- **iOS:**

  - `iphone/Maps/LocalizedStrings/en.lproj/Localizable.strings` – Standard Xcode format for UI strings.
  - `iphone/Maps/LocalizedStrings/en.lproj/Localizable.stringsdict` – Xcode format for handling plurals.

- **SoundText-to-Speech (for navigation):**

  - `data/sound-strings/en.json/localize.json` – JSON localization format, editable in any text editor.

- **Countries (for downloader):**
  - `data/countries-strings/en.json/localize.json` – JSON localization format, editable in any text editor.
