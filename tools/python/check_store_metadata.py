#!/usr/bin/env python3
#
# Check AppStore/GooglePlay metadata
#

import os
import sys
import glob
from urllib.parse import urlparse

os.chdir(os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", ".."))

# https://support.google.com/googleplay/android-developer/answer/9844778?visit_id=637740303439369859-3116807078&rd=1#zippy=%2Cview-list-of-available-languages
GPLAY_LOCALES = [
    "af", "am", "ar", "hy-AM", "az-AZ", "eu-ES", "be", "bn-BD", "bg", "my-MM",
    "ca", "zh-HK", "zh-CN", "zh-TW", "hr", "cs-CZ", "da-DK", "nl-NL", "en-AU",
    "en-CA", "en-IN", "en-SG", "en-GB", "en-US", "et", "fil", "fi-FI", "fr-FR",
    "fr-CA", "gl-ES", "ka-GE", "de-DE", "el-GR", "iw-IL", "hi-IN", "hu-HU",
    "is-IS", "id", "it-IT", "ja-JP", "kn-IN", "km-KH", "ko-KR", "ky-KG",
    "lo-LA", "lv", "lt", "mk-MK", "ms", "ml-IN", "mr-IN", "mn-MN", "ne-NP",
    "no-NO", "fa", "pl-PL", "pt-BR", "pt-PT", "ro", "rm", "ru-RU", "sr",
    "si-LK", "sk", "sl", "es-419", "es-ES", "es-US", "sw", "sv-SE", "ta-IN",
    "te-IN", "th", "tr-TR", "uk", "vi", "zu",
]

# From a Fastline error message
APPSTORE_LOCALES = [
    "ar-SA", "ca", "cs", "da", "de-DE", "el", "en-AU", "en-CA",
    "en-GB", "en-US", "es-ES", "es-MX", "fi", "fr-CA", "fr-FR",
    "he", "hi", "hr", "hu", "id", "it", "ja", "ko", "ms", "nl-NL",
    "no", "pl", "pt-BR", "pt-PT", "ro", "ru", "sk", "sv", "th", "tr",
    "uk", "vi", "zh-Hans", "zh-Hant"
]

def error(path, message, *args, **kwargs):
    print("❌", path + ":", message.format(*args, **kwargs), file=sys.stderr)
    return False


def done(path, ok):
    if ok:
        print("✅", path)
    return ok

def check_raw(path, max_length):
    ok = True
    with open(path, 'r') as f:
        text = f.read()
        if text[-1] == os.linesep:
            text = text[:-1]
        else:
            ok = error(path, "missing new line")
        cur_length = len(text)
        if cur_length > max_length:
            ok = error(path, "too long: got={}, expected={}", cur_length, max_length)
        return ok, text

def check_text(path, max):
    return done(path, check_raw(path, max)[0])

def check_url(path,):
    (ok, url) = check_raw(path, 500)
    url = urlparse(url)
    if not url.scheme in ('https', 'http'):
        ok = error(path, "invalid URL: {}", url)
    return done(path, ok)

def check_email(path):
    (ok, email) = check_raw(path, 500)
    ok = ok and email.find('@') != -1 and email.find('.') != -1
    return done(path, ok)

def check_exact(path, expected):
    (ok, value) = check_raw(path, len(expected))
    if value != expected:
        ok = error(path, "invalid value: got={}, expected={}", value, expected)
    return done(path, ok)

def check_android():
    ok = True
    for flavor in glob.glob('android/src/*/play/'):
        ok = check_url(flavor + "contact-website.txt") and ok
        ok = check_email(flavor + "contact-email.txt") and ok
        ok = check_exact(flavor + "default-language.txt", "en-US") and ok
        maxTitle = 30 if 'google' in flavor else 50
        for locale in glob.glob(flavor + 'listings/*/'):
            if locale.split('/')[-2] not in GPLAY_LOCALES:
                ok = error(locale, "unsupported locale") and ok
                continue
            ok = check_text(locale + "title.txt", maxTitle) and ok
            ok = check_text(locale + "short-description.txt", 80) and ok
            ok = check_text(locale + "full-description.txt", 4000) and ok
        for locale in glob.glob(flavor + 'release-notes/??-??/'):
            ok = check_text(locale + "default.txt", 500) and ok
    return ok

def check_ios():
    ok = True
    for locale in glob.glob('iphone/metadata/*/'):
        if locale.split('/')[-2] not in APPSTORE_LOCALES:
            ok = error(locale, "unsupported locale") and ok
            continue
        ok = check_text(locale + "name.txt", 30) and ok
        ok = check_text(locale + "subtitle.txt", 30) and ok
        ok = check_text(locale + "promotional_text.txt", 170) and ok
        ok = check_text(locale + "description.txt", 4000) and ok
        ok = check_text(locale + "release_notes.txt", 4000) and ok
        ok = check_text(locale + "keywords.txt", 100) and ok
        ok = check_url(locale + "support_url.txt") and ok
        ok = check_url(locale + "marketing_url.txt") and ok
    return ok

if __name__ == "__main__":
    ok = True
    if len(sys.argv) == 2 and sys.argv[1] == 'android':
        if check_android():
            sys.exit(0)
        sys.exit(2)
    elif len(sys.argv) == 2 and sys.argv[1] == "ios":
        if check_ios():
            sys.exit(0)
        sys.exit(2)
    else:
       print("Usage:", sys.argv[0], "android|ios", file=sys.stderr)  
       sys.exit(1)
