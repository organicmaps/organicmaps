#!/usr/bin/env python3
#
# Check AppStore/GooglePlay metadata
#

import os
import re
import sys
import glob
import shutil
from urllib.parse import urlparse

os.chdir(os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", ".."))

# https://support.google.com/googleplay/android-developer/answer/9844778?visit_id=637740303439369859-3116807078&rd=1#zippy=%2Cview-list-of-available-languages
GPLAY_LOCALES = [
    "af",    # Afrikaans
    "sq",    # Albanian
    "am",    # Amharic
    "ar",    # Arabic
    "hy-AM", # Armenian
    "az-AZ", # Azerbaijani
    "bn-BD", # Bangla
    "eu-ES", # Basque
    "be",    # Belarusian
    "bg",    # Bulgarian
    "my-MM", # Burmese
    "ca",    # Catalan
    "zh-HK", # Chinese (Hong Kong)
    "zh-CN", # Chinese (Simplified)
    "zh-TW", # Chinese (Traditional)
    "hr",    # Croatian
    "cs-CZ", # Czech
    "da-DK", # Danish
    "nl-NL", # Dutch
    "en-IN", # English
    "en-SG", # English
    "en-ZA", # English
    "en-AU", # English (Australia)
    "en-CA", # English (Canada)
    "en-GB", # English (United Kingdom)
    "en-US", # English (United States)
    "et",    # Estonian
    "fil",   # Filipino
    "fi-FI", # Finnish
    "fr-CA", # French (Canada)
    "fr-FR", # French (France)
    "gl-ES", # Galician
    "ka-GE", # Georgian
    "de-DE", # German
    "el-GR", # Greek
    "gu",    # Gujarati
    "iw-IL", # Hebrew
    "hi-IN", # Hindi
    "hu-HU", # Hungarian
    "is-IS", # Icelandic
    "id",    # Indonesian
    "it-IT", # Italian
    "ja-JP", # Japanese
    "kn-IN", # Kannada
    "kk",    # Kazakh
    "km-KH", # Khmer
    "ko-KR", # Korean
    "ky-KG", # Kyrgyz
    "lo-LA", # Lao
    "lv",    # Latvian
    "lt",    # Lithuanian
    "mk-MK", # Macedonian
    "ms",    # Malay
    "ms-MY", # Malay (Malaysia)
    "ml-IN", # Malayalam
    "mr-IN", # Marathi
    "mn-MN", # Mongolian
    "ne-NP", # Nepali
    "no-NO", # Norwegian
    "fa",    # Persian
    "fa-AE", # Persian
    "fa-AF", # Persian
    "fa-IR", # Persian
    "pl-PL", # Polish
    "pt-BR", # Portuguese (Brazil)
    "pt-PT", # Portuguese (Portugal)
    "pa",    # Punjabi
    "ro",    # Romanian
    "rm",    # Romansh
    "ru-RU", # Russian
    "sr",    # Serbian
    "si-LK", # Sinhala
    "sk",    # Slovak
    "sl",    # Slovenian
    "es-419", # Spanish (Latin America)
    "es-ES", # Spanish (Spain)
    "es-US", # Spanish (United States)
    "sw",    # Swahili
    "sv-SE", # Swedish
    "ta-IN", # Tamil
    "te-IN", # Telugu
    "th",    # Thai
    "tr-TR", # Turkish
    "uk",    # Ukrainian
    "ur",    # Urdu
    "vi",    # Vietnamese
    "zu",    # Zulu
]

# Locales supported by Huawei AppGallery but not Google Play.
HUAWEI_ONLY_LOCALES = [
    "as",    # Assamese
    "bo",    # Tibetan
    "bs",    # Bosnian
    "jv",    # Javanese
    "mai",   # Maithili
    "mi",    # Maori
    "or",    # Oriya
    "ug",    # Uighur
    "uz",    # Uzbek
]

# From a Fastline error message and https://help.apple.com/app-store-connect/#/dev997f9cf7c
APPSTORE_LOCALES = [
    "ar-SA", "bn-BD", "ca", "cs", "da", "de-DE", "el", "en-AU",
    "en-CA", "en-GB", "en-US", "es-ES", "es-MX", "fi", "fr-CA", "fr-FR",
    "gu-IN", "he", "hi", "hr", "hu", "id", "it", "ja", "kn-IN",
    "ko", "ml-IN", "mr-IN", "ms", "nl-NL", "no", "or-IN", "pa-IN",
    "pl", "pt-BR", "pt-PT", "ro", "ru", "sk", "sl-SI", "sv", "ta-IN",
    "te-IN", "th", "tr", "uk", "ur-PK", "vi", "zh-Hans", "zh-Hant"
]

# Combined Android locale set for O(1) membership tests in check_path().
ANDROID_LOCALES = frozenset(GPLAY_LOCALES + HUAWEI_ONLY_LOCALES)

# Char limits for files validated by check_path(). Source of truth for the
# `files` subcommand; the bulk check_android()/check_ios() use literals inline.
ANDROID_LISTING_LIMITS = {
    'title.txt': 50,
    'title-google.txt': 30,
    'short-description.txt': 80,
    'short-description-google.txt': 80,
    'full-description.txt': 4000,
    'full-description-google.txt': 4000,
    'release-notes.txt': 500,
}
IOS_METADATA_LIMITS = {
    'name.txt': 30,
    'subtitle.txt': 30,
    'promotional_text.txt': 170,
    'description.txt': 4000,
    'release_notes.txt': 4000,
    'keywords.txt': 100,
}
IOS_URL_FILES = ('support_url.txt', 'marketing_url.txt', 'privacy_url.txt')

# Pre-compiled path patterns dispatched by check_path().
_LISTINGS_RE = re.compile(r'^android/app/src/(?:fdroid|google)/play/listings/([^/]+)/([^/]+\.txt)$')
_RELEASE_NOTES_RE = re.compile(r'^android/app/src/(?:fdroid|google)/play/release-notes/([^/]+)/([^/]+\.txt)$')
_PLAY_TOPLEVEL_RE = re.compile(r'^android/app/src/(?:fdroid|google)/play/([^/]+\.txt)$')
_IOS_METADATA_RE = re.compile(r'^iphone/metadata/([^/]+)/([^/]+\.txt)$')


def contains_emoji(text):
    """Heuristic emoji detection avoiding false positives for CJK, Korean, Thai.

    We purposefully restrict to well‑known emoji code point ranges and symbols
    that are almost never used as plain alphabetic text in the app metadata.
    This avoids broad regex character classes that accidentally match
    non‑emoji letters in some environments.

    Ranges covered (inclusive):
      * 0x1F1E6–0x1F1FF (regional indicator symbols / flags)
      * 0x1F300–0x1FAFF (primary emoji blocks incl. supplemental)
      * 0x2600–0x26FF (misc symbols) – classic sun, cloud, etc.
      * 0x2700–0x27BF (dingbats) – arrows, stars, etc.
      * Variation Selector-16 (U+FE0F) when following an emoji base
      * Zero Width Joiner sequences (U+200D) – if part of a chain containing
        an emoji code point.
    """
    prev_was_emoji = False
    for ch in text:
        cp = ord(ch)
        is_base = (
            0x1F1E6 <= cp <= 0x1F1FF or
            0x1F300 <= cp <= 0x1FAFF or
            0x2600 <= cp <= 0x26FF or
            0x2700 <= cp <= 0x27BF
        )
        if is_base:
            return True
        # Variation Selector-16 turns some symbols into emoji presentation.
        if cp == 0xFE0F and prev_was_emoji:
            return True
        # Zero Width Joiner inside an emoji sequence.
        if cp == 0x200D and prev_was_emoji:
            # Keep prev_was_emoji True to allow continuation.
            continue
        prev_was_emoji = is_base
    return False


def error(path, message, *args, **kwargs):
    print("❌", path + ":", message.format(*args, **kwargs), file=sys.stderr)
    return False


def done(path, ok):
    if ok:
        print("✅", path)
    return ok

def check_raw(path, max_length, fix=False):
    ok = True
    with open(path, 'rb') as f:
        raw = f.read()
    text = raw.decode('utf-8').rstrip()
    if fix and (encoded := text.encode('utf-8')) != raw:
        with open(path, 'wb') as f:
            f.write(encoded)
        print("✂️ ", path + ":", "trimmed trailing whitespace")

    if contains_emoji(text):
        ok = error(path, "contains emoji characters")

    if len(text) > max_length:
        ok = error(path, "too long: got={}, expected={}", len(text), max_length)
    return ok, text

def check_text(path, max, optional=False, fix=False):
    try:
        return done(path, check_raw(path, max, fix=fix)[0])
    except FileNotFoundError as e:
        if optional:
            return True,
        print("🚫", path)
        return False,

def check_url(path, fix=False):
    (ok, url) = check_raw(path, 500, fix=fix)
    url = urlparse(url)
    if not url.scheme in ('https', 'http'):
        ok = error(path, "invalid URL: {}", url)
    return done(path, ok)

def check_email(path, fix=False):
    (ok, email) = check_raw(path, 500, fix=fix)
    ok = ok and email.find('@') != -1 and email.find('.') != -1
    return done(path, ok)

def check_exact(path, expected, fix=False):
    (ok, value) = check_raw(path, len(expected), fix=fix)
    if value != expected:
        ok = error(path, "invalid value: got={}, expected={}", value, expected)
    return done(path, ok)


def check_android():
    ok = True
    flavor = 'android/app/src/fdroid/play/'
    ok = check_url(flavor + 'contact-website.txt') and ok
    ok = check_email(flavor + 'contact-email.txt') and ok
    ok = check_exact(flavor + 'default-language.txt', 'en-US') and ok
    all_locales = set(GPLAY_LOCALES + HUAWEI_ONLY_LOCALES)
    for locale in glob.glob(flavor + 'listings/*/'):
        if locale.split('/')[-2] not in all_locales:
            ok = error(locale, 'unsupported locale') and ok
            continue
        ok = check_text(locale + 'title.txt', 50) and ok
        ok = check_text(locale + 'title-google.txt', 30) and ok
        ok = check_text(locale + 'short-description.txt', 80) and ok
        ok = check_text(locale + 'short-description-google.txt', 80, True) and ok
        ok = check_text(locale + 'full-description.txt', 4000) and ok
        ok = check_text(locale + 'full-description-google.txt', 4000, True) and ok
        ok = check_text(locale + 'release-notes.txt', 500) and ok
    for locale in glob.glob(flavor + 'release-notes/*/'):
        if locale.split('/')[-2] not in all_locales:
            ok = error(locale, 'unsupported locale') and ok
            continue
        ok = check_text(locale + 'default.txt', 500) and ok
    return ok


def check_ios():
    ok = True
    for locale in APPSTORE_LOCALES:
        locale_dir = os.path.join('iphone', 'metadata', locale)
        english_dir = os.path.join('iphone', 'metadata', 'en-US')
        overlay_dir = os.path.join('keywords', 'ios', locale)
        if not os.path.isdir(locale_dir):
            os.mkdir(locale_dir)
        for name in ["name.txt", "subtitle.txt", "promotional_text.txt",
                        "description.txt", "release_notes.txt", "keywords.txt",
                        "support_url.txt", "marketing_url.txt", "privacy_url.txt"]:
            overlay_path = os.path.join(overlay_dir, name)
            english_path = os.path.join(english_dir, name)
            target_path = os.path.join(locale_dir, name)
            if os.path.exists(overlay_path):
                shutil.copy(overlay_path, target_path)
            elif os.path.exists(english_path) and not os.path.exists(target_path):
                shutil.copy(english_path, target_path)

    SKIP_DIRS = {'review_information'}
    for locale in glob.glob('iphone/metadata/*/'):
        dirname = locale.split('/')[-2]
        if dirname in SKIP_DIRS:
            continue
        if dirname not in APPSTORE_LOCALES:
            ok = error(locale, "unsupported locale") and ok
            continue
        ok = check_text(locale + "name.txt", 30) and ok
        ok = check_text(locale + "subtitle.txt", 30) and ok
        ok = check_text(locale + "promotional_text.txt", 170) and ok
        ok = check_text(locale + "description.txt", 4000) and ok
        ok = check_text(locale + "release_notes.txt", 4000) and ok
        ok = check_text(locale + "keywords.txt", 100, True) and ok
        ok = check_url(locale + "support_url.txt") and ok
        ok = check_url(locale + "marketing_url.txt") and ok
        ok = check_url(locale + "privacy_url.txt") and ok
    for locale in glob.glob('keywords/ios/*/'):
        if locale.split('/')[-2] not in APPSTORE_LOCALES:
            ok = error(locale, "unsupported locale") and ok
            continue
    return ok

def check_path(path, fix=False):
    """Run the appropriate check on a single file based on its path.

    Used by the `files` subcommand (e.g. from the pre-commit hook) to validate
    only the files staged for commit instead of the whole tree. Unknown paths
    are skipped (return True) so non-metadata files in the same commit are a
    no-op rather than an error.
    """
    # Android Play Store listings (fdroid or google flavor share the same layout).
    if m := _LISTINGS_RE.match(path):
        locale, name = m[1], m[2]
        if locale not in ANDROID_LOCALES:
            return error(path, 'unsupported locale: {}', locale)
        if name in ANDROID_LISTING_LIMITS:
            return done(path, check_raw(path, ANDROID_LISTING_LIMITS[name], fix=fix)[0])
        if name == 'video-url.txt':
            return check_url(path, fix=fix)
        return True

    # Per-track release notes.
    if m := _RELEASE_NOTES_RE.match(path):
        locale, name = m[1], m[2]
        if locale not in ANDROID_LOCALES:
            return error(path, 'unsupported locale: {}', locale)
        if name == 'default.txt':
            return done(path, check_raw(path, 500, fix=fix)[0])
        return True

    # Top-level Play Store metadata.
    if m := _PLAY_TOPLEVEL_RE.match(path):
        match m[1]:
            case 'contact-website.txt': return check_url(path, fix=fix)
            case 'contact-email.txt':   return check_email(path, fix=fix)
            case 'default-language.txt': return check_exact(path, 'en-US', fix=fix)
        return True

    # iOS metadata.
    if m := _IOS_METADATA_RE.match(path):
        locale, name = m[1], m[2]
        if locale == 'review_information':
            return True
        if locale not in APPSTORE_LOCALES:
            return error(path, 'unsupported locale: {}', locale)
        if name in IOS_METADATA_LIMITS:
            return done(path, check_raw(path, IOS_METADATA_LIMITS[name], fix=fix)[0])
        if name in IOS_URL_FILES:
            return check_url(path, fix=fix)
        return True

    return True  # Not a metadata file we know about — skip.


if __name__ == "__main__":
    usage = f"Usage: {sys.argv[0]} android|ios|files [--fix] FILE [FILE...]"
    match sys.argv[1:]:
        case ['android']:
            sys.exit(0 if check_android() else 2)
        case ['ios']:
            sys.exit(0 if check_ios() else 2)
        case ['files', *rest] if (paths := [a for a in rest if a != '--fix']):
            fix = '--fix' in rest
            # List comp (not generator) so every path is checked even after a failure.
            sys.exit(0 if all([check_path(p, fix=fix) for p in paths]) else 2)
        case _:
            print(usage, file=sys.stderr)
            sys.exit(1)
