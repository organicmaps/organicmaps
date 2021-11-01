#!/usr/bin/env python3

import sys
from googletrans import Translator

LANGUAGES = [
    "ar",
    "be",
    "bg",
    "cs",
    "da",
    "de",
    "el",
    "en",
    "es",
    "fa",
    "fi",
    "fr",
    "he",
    "hu",
    "id",
    "it",
    "ja",
    "ko",
    "nl",
    "no",
    "pl",
    "pt",
    "ro",
    "ru",
    "sk",
    "sv",
    "sw",
    "th",
    "tr",
    "uk",
    "vi",
    "zh-cn",
    "zh-tw"
]

STRINGS_OVERRIDE = {
    "no": "nb",
    "zh-cn": "zh-Hans",
    "zh-tw": "zh-Hant",
}

if len(sys.argv) < 2:
    print("Usage:", sys.argv[0], "STRING", file=sys.stderr)
    sys.exit(1)

text = sys.argv[1]
src = 'en'

translator = Translator()
for dest in LANGUAGES:
    trans = translator.translate(text, src=src, dest=dest).text
    # Correct language codes to ours.
    dest = STRINGS_OVERRIDE.get(dest, dest)
    print(dest, '=', trans)
