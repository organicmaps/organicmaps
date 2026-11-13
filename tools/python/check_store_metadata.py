#!/usr/bin/env python3
#
# Check AppStore/GooglePlay metadata
#

import os
import re
import sys
import glob
import json
import shutil
from pathlib import Path
from urllib.parse import urlparse

# https://support.google.com/googleplay/android-developer/answer/9844778?visit_id=637740303439369859-3116807078&rd=1#zippy=%2Cview-list-of-available-languages
GPLAY_LOCALES = json.loads(Path(__file__).with_name('google_play_locales.json').read_text())

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

# Char limits for files validated by check_path() and check_google().
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
_LISTINGS_RE = re.compile(r'^android/app/src/(fdroid|google)/play/listings/([^/]+)/([^/]+\.txt)$')
_RELEASE_NOTES_RE = re.compile(r'^android/app/src/(fdroid|google)/play/release-notes/([^/]+)/([^/]+\.txt)$')
_PLAY_TOPLEVEL_RE = re.compile(r'^android/app/src/(?:fdroid|google)/play/([^/]+\.txt)$')
_IOS_METADATA_RE = re.compile(r'^iphone/metadata/([^/]+)/([^/]+\.txt)$')
# Additional text-style symbols require VS16; plain copyright signs and arrows are valid.
# Emoji properties: https://www.unicode.org/Public/18.0.0/ucd/emoji/emoji-data.txt
_EMOJI_RE = re.compile(
    r'[\U0001F1E6-\U0001F1FF\U0001F300-\U0001FAFF'
    r'\u2600-\u2604\u260E\u2611\u2614\u2615\u2618\u261D\u2620\u2622\u2623\u2626\u262A\u262E\u262F'
    r'\u2638-\u263A\u2640\u2642\u2648-\u2653\u265F\u2660\u2663\u2665\u2666\u2668\u267B\u267E\u267F'
    r'\u2692-\u2697\u2699\u269B\u269C\u26A0\u26A1\u26A7\u26AA\u26AB\u26B0\u26B1\u26BD\u26BE'
    r'\u26C4\u26C5\u26C8\u26CE\u26CF\u26D1\u26D3\u26D4\u26E9\u26EA\u26F0-\u26F5\u26F7-\u26FA\u26FD'
    r'\u2702\u2705\u2708-\u270D\u270F\u2712\u2714\u2716\u271D\u2721\u2728\u2733\u2734\u2744\u2747'
    r'\u274C\u274E\u2753-\u2755\u2757\u2763\u2764\u2795-\u2797\u27A1\u27B0\u27BF'
    r'\u231A-\u231B\u23E9-\u23EC\u23F0\u23F3\u25FD-\u25FE\u2B1B-\u2B1C\u2B50\u2B55\U0001F004'
    r'\U0001F0CF\U0001F18E\U0001F191-\U0001F19A\U0001F201\U0001F21A\U0001F22F\U0001F232-\U0001F236'
    r'\U0001F238-\U0001F23A\U0001F250-\U0001F251]'
    r'|[#*0-9]\uFE0F?\u20E3'
    r'|[\u00A9\u00AE\u203C\u2049\u2122\u2139\u2194-\u2199\u21A9-\u21AA\u2328\u23CF\u23ED-\u23EF'
    r'\u23F1-\u23F2\u23F8-\u23FA\u24C2\u25AA-\u25AB\u25B6\u25C0\u25FB-\u25FC\u2934-\u2935'
    r'\u2B05-\u2B07\u3030\u303D\u3297\u3299\U0001F170-\U0001F171\U0001F17E-\U0001F17F\U0001F202\U0001F237]'
    r'\uFE0F'
)


def contains_emoji(text):
    """Detect emoji characters, presentation variants and keycaps."""
    return bool(_EMOJI_RE.search(text))


def error(path, message, *args, **kwargs):
    print("❌", path + ":", message.format(*args, **kwargs), file=sys.stderr)
    return False


def done(path, ok):
    if ok:
        print("✅", path)
    return ok

def check_raw(path, max_length, fix=False):
    ok = True
    try:
        with open(path, 'rb') as f:
            raw = f.read()
    except FileNotFoundError:
        print("🚫", path)
        return False, ''
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
    if optional and not os.path.exists(path):
        return True
    return done(path, check_raw(path, max, fix=fix)[0])

def check_url(path, fix=False):
    (ok, url) = check_raw(path, 500, fix=fix)
    if not ok:
        return False
    url = urlparse(url)
    if url.scheme not in ('https', 'http'):
        ok = error(path, "invalid URL: {}", url)
    return done(path, ok)

def check_email(path, fix=False):
    (ok, email) = check_raw(path, 500, fix=fix)
    ok = ok and email.find('@') != -1 and email.find('.') != -1
    return done(path, ok)

def check_exact(path, expected, fix=False):
    (ok, value) = check_raw(path, len(expected), fix=fix)
    if ok and value != expected:
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
    return check_google() and ok


def check_google(play_dir=Path('android/app/src/google/play'), generated=False):
    """Validate Google source or merged publishing resources without resolving symlinks."""
    play_dir = Path(play_dir)
    ok = True
    notes_dir = play_dir / 'release-notes'
    if not (notes_dir / 'en-US' / 'default.txt').is_file():
        ok = error(str(notes_dir), 'missing en-US/default.txt')
    for category in ('listings', 'release-notes'):
        directory = play_dir / category
        if not directory.exists():
            continue
        for locale in sorted(directory.iterdir()):
            if locale.name.startswith('.'):
                continue
            # GPP can leave empty locale directories after pruning deleted notes.
            if generated and category == 'release-notes' and locale.is_dir() and not list(locale.glob('*.txt')):
                continue
            if locale.name not in GPLAY_LOCALES:
                ok = error(str(locale), 'unsupported Google Play locale') and ok
                continue
            if not locale.is_dir():
                ok = error(str(locale), 'expected a locale directory (or valid symlink)') and ok
                continue
            files = sorted(p for p in locale.glob('*.txt') if not p.name.startswith('.'))
            if category == 'release-notes' and not files:
                ok = error(str(locale), 'missing release notes') and ok
            for path in files:
                if category == 'release-notes':
                    ok = check_text(str(path), 500) and ok
                elif path.name in ANDROID_LISTING_LIMITS:
                    limit = 30 if path.name == 'title.txt' else ANDROID_LISTING_LIMITS[path.name]
                    ok = check_text(str(path), limit) and ok
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
        flavor, locale, name = m[1], m[2], m[3]
        if locale not in (GPLAY_LOCALES if flavor == 'google' else ANDROID_LOCALES):
            return error(path, 'unsupported locale: {}', locale)
        if name in ANDROID_LISTING_LIMITS:
            limit = 30 if flavor == 'google' and name == 'title.txt' else ANDROID_LISTING_LIMITS[name]
            return done(path, check_raw(path, limit, fix=fix)[0])
        if name == 'video-url.txt':
            return check_url(path, fix=fix)
        return True

    # Per-track release notes.
    if m := _RELEASE_NOTES_RE.match(path):
        flavor, locale = m[1], m[2]
        if locale not in (GPLAY_LOCALES if flavor == 'google' else ANDROID_LOCALES):
            return error(path, 'unsupported locale: {}', locale)
        return done(path, check_raw(path, 500, fix=fix)[0])

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
    os.chdir(Path(__file__).resolve().parents[2])
    usage = f"Usage: {sys.argv[0]} android|ios|google [PLAY_DIR | --generated PLAY_DIR]|files [--fix] FILE [FILE...]"
    match sys.argv[1:]:
        case ['android']:
            sys.exit(0 if check_android() else 2)
        case ['ios']:
            sys.exit(0 if check_ios() else 2)
        case ['google']:
            sys.exit(0 if check_google() else 2)
        case ['google', '--generated', play_dir]:
            sys.exit(0 if check_google(Path(play_dir), generated=True) else 2)
        case ['google', play_dir]:
            sys.exit(0 if check_google(Path(play_dir)) else 2)
        case ['files', *rest] if (paths := [a for a in rest if a != '--fix']):
            fix = '--fix' in rest
            # List comp (not generator) so every path is checked even after a failure.
            sys.exit(0 if all([check_path(p, fix=fix) for p in paths]) else 2)
        case _:
            print(usage, file=sys.stderr)
            sys.exit(1)
