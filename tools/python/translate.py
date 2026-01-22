#!/usr/bin/env python3

# Requires `brew install translate-shell`
# and DEEPL_FREE_API_KEY or DEEPL_API_KEY environment variables set.

import argparse
import os
import platform
import shutil
import subprocess
import sys
import time
from typing import Optional, Dict, List

import requests

TRANS_CMD = "trans"

# By default, most languages use formal translations.
# Languages marked with * in DeepL docs don't support formality.
INFORMAL_LANGUAGES = ["pt-BR", "de"]

# Languages that don't support formality in DeepL (marked with * in their docs).
# These languages only work with the quality_optimized model or when no model is specified.
DEEPL_NO_FORMALITY_LANGUAGES = [
    "af",
    "an",
    "as",
    "ay",
    "az",
    "ba",
    "be",
    "bho",
    "bn",
    "br",
    "bs",
    "ca",
    "ceb",
    "ckb",
    "cy",
    "eo",
    "eu",
    "fa",
    "ga",
    "gl",
    "gn",
    "gom",
    "gu",
    "ha",
    "hi",
    "hr",
    "ht",
    "hy",
    "ig",
    "is",
    "jv",
    "ka",
    "kk",
    "kmr",
    "ky",
    "la",
    "lb",
    "lmo",
    "ln",
    "mai",
    "mg",
    "mi",
    "mk",
    "ml",
    "mn",
    "mr",
    "ms",
    "mt",
    "my",
    "ne",
    "oc",
    "om",
    "pa",
    "pag",
    "pam",
    "prs",
    "ps",
    "qu",
    "sa",
    "scn",
    "sq",
    "sr",
    "st",
    "su",
    "sw",
    "ta",
    "te",
    "tg",
    "th",
    "tk",
    "tl",
    "tn",
    "ts",
    "tt",
    "ur",
    "uz",
    "vi",
    "wo",
    "xh",
    "yi",
    "yue",
    "zu",
    "ace",
]

# Use DeepL when possible with a fall back to Google.
# Google is only used as a fallback when DeepL doesn't translate or for unsupported languages.
# List of Google Translate target languages: https://cloud.google.com/translate/docs/languages
GOOGLE_TARGET_LANGUAGES = [
    # Languages that might need Google fallback if DeepL fails
    "ar",
    "bg",
    "cs",
    "da",
    "de",
    "el",
    "es",
    "et",
    "fi",
    "fr",
    "hu",
    "id",
    "it",
    "ja",
    "ko",
    "lt",
    "lv",
    "nb",
    "nl",
    "pl",
    "pt-BR",
    "ro",
    "ru",
    "sk",
    "sv",
    "tr",
    "uk",
    "zh-CN",  # zh-Hans in OM
    "zh-TW",  # zh-Hant in OM
    # Additional languages supported by Google but mapping to DeepL codes
    "af",
    "az",
    "be",
    "ca",
    "eu",
    "fa",
    "he",
    "hi",
    "mr",
    "sw",
    "th",
    "vi",
]

# See https://developers.deepl.com/docs/getting-started/supported-languages for target languages.
# Full list of DeepL target languages as of 2025.
# Languages not supported by Organic Maps are commented out.
DEEPL_TARGET_LANGUAGES = [
    # "ace",  # Acehnese
    "af",  # Afrikaans
    # "an",  # Aragonese
    "ar",  # Arabic
    # "as",  # Assamese
    # "ay",  # Aymara
    "az",  # Azerbaijani
    # "ba",  # Bashkir
    "be",  # Belarusian
    "bg",  # Bulgarian
    # "bho",  # Bhojpuri
    "bn",  # Bengali
    # "br",  # Breton
    # "bs",  # Bosnian
    "ca",  # Catalan
    # "ceb",  # Cebuano
    # "ckb",  # Kurdish (Sorani)
    "cs",  # Czech
    # "cy",  # Welsh
    "da",  # Danish
    "de",  # German
    "el",  # Greek
    "en-GB",  # English (British)
    "en-US",  # English (American) - en in OM
    # "eo",  # Esperanto
    "es",  # Spanish
    "es-419",  # Spanish (Latin American) - es-MX in OM
    "et",  # Estonian
    "eu",  # Basque
    "fa",  # Persian
    "fi",  # Finnish
    "fr",  # French
    # "ga",  # Irish
    "gl",  # Galician
    # "gn",  # Guarani
    # "gom",  # Konkani
    # "gu",  # Gujarati
    # "ha",  # Hausa
    "he",  # Hebrew
    "hi",  # Hindi
    "hr",  # Croatian
    # "ht",  # Haitian Creole
    "hu",  # Hungarian
    # "hy",  # Armenian
    "id",  # Indonesian
    # "ig",  # Igbo
    "is",  # Icelandic
    "it",  # Italian
    "ja",  # Japanese
    # "jv",  # Javanese
    # "ka",  # Georgian
    # "kk",  # Kazakh
    # "kmr",  # Kurdish (Kurmanji)
    "ko",  # Korean
    # "ky",  # Kyrgyz
    # "la",  # Latin
    # "lb",  # Luxembourgish
    # "lmo",  # Lombard
    # "ln",  # Lingala
    "lt",  # Lithuanian
    "lv",  # Latvian
    # "mai",  # Maithili
    # "mg",  # Malagasy
    # "mi",  # Maori
    # "mk",  # Macedonian
    # "ml",  # Malayalam
    # "mn",  # Mongolian
    "mr",  # Marathi
    # "ms",  # Malay
    "mt",  # Maltese
    # "my",  # Burmese
    "nb",  # Norwegian Bokmål
    # "ne",  # Nepali
    "nl",  # Dutch
    # "oc",  # Occitan
    # "om",  # Oromo
    "pa",  # Punjabi
    # "pag",  # Pangasinan
    # "pam",  # Kapampangan
    "pl",  # Polish
    # "prs",  # Dari
    # "ps",  # Pashto
    "pt-BR",  # Portuguese (Brazilian)
    "pt-PT",  # Portuguese (European) - pt in OM
    # "qu",  # Quechua
    "ro",  # Romanian
    "ru",  # Russian
    # "sa",  # Sanskrit
    # "scn",  # Sicilian
    "sk",  # Slovak
    "sl",  # Slovenian
    "sq",  # Albanian
    "sr",  # Serbian
    # "st",  # Sesotho
    # "su",  # Sundanese
    "sv",  # Swedish
    "sw",  # Swahili
    # "ta",  # Tamil
    # "te",  # Telugu
    # "tg",  # Tajik
    "th",  # Thai
    # "tk",  # Turkmen
    # "tl",  # Tagalog
    # "tn",  # Tswana
    "tr",  # Turkish
    # "ts",  # Tsonga
    # "tt",  # Tatar
    "uk",  # Ukrainian
    # "ur",  # Urdu
    # "uz",  # Uzbek
    "vi",  # Vietnamese
    # "wo",  # Wolof
    # "xh",  # Xhosa
    # "yi",  # Yiddish
    # "yue",  # Cantonese
    "zh-Hans",  # Chinese (simplified)
    "zh-Hant",  # Chinese (traditional)
    # "zu",  # Zulu
]
GOOGLE_TARGET_LANGUAGES = list(set(GOOGLE_TARGET_LANGUAGES))  # Remove duplicates
GOOGLE_TARGET_LANGUAGES.sort()


def get_api_key() -> str:
    key = os.environ.get("DEEPL_FREE_API_KEY")
    if key is None:
        key = os.environ.get("DEEPL_API_KEY")
    if key is None:
        print("Error: DEEPL_FREE_API_KEY or DEEPL_API_KEY env variables are not set")
        exit(1)
    return key


def google_translate(text: str, source_language: str) -> Dict[str, str]:
    fromTo = source_language.lower() + ":"
    # Translate all languages with Google to replace failed DeepL translations.
    for lang in GOOGLE_TARGET_LANGUAGES:
        fromTo += lang + "+"
    # Remove last +
    fromTo = fromTo[:-1]
    res = subprocess.run(
        [TRANS_CMD, "-b", "-no-bidi", fromTo, text], text=True, capture_output=True
    )
    if res.returncode != 0:
        print(f"Error running {TRANS_CMD} program:")
        print(res.stderr)
        exit(1)

    print("\nGoogle translations:")
    translations = {}
    i = 0
    for line in res.stdout.splitlines():
        lang = GOOGLE_TARGET_LANGUAGES[i]
        # Map Google language codes to OM language codes
        om_lang = lang
        if lang == "zh-TW":
            om_lang = "zh-Hant"
        elif lang == "pt-PT":
            om_lang = "pt"
        elif lang == "zh-CN":
            om_lang = "zh-Hans"
        translations[om_lang] = line
        i = i + 1
        print(om_lang + " = " + line)
    return translations


def google_translate_one(text: str, source_language: str, target_language: str) -> str:
    fromTo = source_language.lower() + ":" + target_language.lower()
    res = subprocess.run(
        [TRANS_CMD, "-b", "-no-bidi", fromTo, text], text=True, capture_output=True
    )
    if res.returncode != 0:
        print(f"Error running {TRANS_CMD} program:")
        print(res.stderr)
        exit(1)
    return res.stdout.splitlines()[0]


def deepl_translate_one(
    text: str, source_language: str, target_language: str, context: Optional[str] = None
) -> str:
    url = "https://api-free.deepl.com/v2/translate"
    # Normalize target language for formality check (lowercase, no region)
    target_lang_base = target_language.lower().split("-")[0]
    payload = {
        "text": text,
        "source_lang": source_language.lower(),
        "target_lang": target_language,
        "enable_beta_languages": True,
        "preserve_formatting": True,
    }
    if context:
        payload["context"] = context

    # Only add formality for languages that support it
    if target_lang_base not in DEEPL_NO_FORMALITY_LANGUAGES:
        payload["formality"] = (
            "prefer_less" if target_language in INFORMAL_LANGUAGES else "prefer_more"
        )
    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Authorization": "DeepL-Auth-Key " + get_api_key(),
    }

    max_retries = 5
    retry_delay = 1.0  # seconds

    for attempt in range(max_retries + 1):
        response = requests.request("POST", url, headers=headers, data=payload)
        if response.status_code == 200:
            json = response.json()
            return json["translations"][0]["text"]
        elif response.status_code == 429:
            if attempt < max_retries:
                print(
                    f"Warning: DeepL rate limit exceeded (429). Retrying in {retry_delay} seconds..."
                )
                time.sleep(retry_delay)
                retry_delay *= 2
                continue
            else:
                print("Error: DeepL rate limit exceeded after maximum retries.")
                # Fall through to error exit

        # Fallback error handling
        print(
            f"Error: DeepL API request failed with status code {response.status_code}"
        )
        print("Response:", response.text)
        exit(1)


def translate_one(
    text: str, source_language: str, target_language: str, context: Optional[str] = None
) -> str:
    # Check if target_language is in DeepL list (case-insensitive)
    deepl_languages_lower = [lang.lower() for lang in DEEPL_TARGET_LANGUAGES]
    if target_language.lower() in deepl_languages_lower:
        # Find the correct case version
        idx = deepl_languages_lower.index(target_language.lower())
        return deepl_translate_one(
            text, source_language, DEEPL_TARGET_LANGUAGES[idx], context=context
        )
    elif target_language in GOOGLE_TARGET_LANGUAGES:
        return google_translate_one(text, source_language, target_language)
    else:
        raise ValueError(f"Unsupported target language {target_language}")


def deepl_translate(
    text: str, source_language: str, context: Optional[str] = None
) -> Dict[str, str]:
    translations = {}
    print("Deepl translations:")
    for lang in DEEPL_TARGET_LANGUAGES:
        translation = deepl_translate_one(text, source_language, lang, context=context)
        # Map DeepL language codes to OM language codes
        om_lang = lang
        if lang == "pt-PT":
            om_lang = "pt"
        elif lang == "en-US":
            om_lang = "en"
        elif lang == "es-419":
            om_lang = "es-MX"
        translations[om_lang] = translation
        print(om_lang + " = " + translation)
    return translations


# Returns a list of all languages supported by the core (search) in data/categories.txt
def get_supported_categories_txt_languages() -> List[str]:
    script_dir = os.path.dirname(os.path.realpath(__file__))
    categories_txt_path = os.path.join(script_dir, "..", "..", "data", "categories.txt")
    languages = set()
    with open(categories_txt_path) as f:
        for line in f.readlines():
            if not line:
                continue
            if line[0] == "#":
                continue
            colon_index = line.find(":")
            # en: en-US: zh-Hant:
            if colon_index == 2 or colon_index == 5 or colon_index == 7:
                languages.add(line[:colon_index])

    # Convert to list.
    languages = list(languages)
    languages.sort()
    return languages


def parse_args(app_name) -> tuple[str, Optional[str]]:
    """
    Parse command line arguments
    :return: text to translate and optional context string
    """
    description = f"""For a custom source language add a two-letter code with a colon in the beginning:
    {app_name} de:Some German text to translate

Supported DeepL languages: {', '.join(DEEPL_TARGET_LANGUAGES)}
Supported Google languages: {', '.join(GOOGLE_TARGET_LANGUAGES)}
"""

    parser = argparse.ArgumentParser(
        epilog=description, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--context", help="Additional context for translation")
    parser.add_argument("text", help="Some English text to translate.")
    args, unknownargs = parser.parse_known_args()
    text = args.text + " " + " ".join(unknownargs)
    return text, args.context


def main(text_to_translate: str, context: str):
    source_language = "en"
    if len(text_to_translate) > 3 and text_to_translate[2] == ":":
        source_language = text_to_translate[0:2]
        text_to_translate = text_to_translate[3:].lstrip()

    translations = deepl_translate(text_to_translate, source_language, context=context)
    google_translations = google_translate(text_to_translate, source_language)
    # Check if DeepL did not translate the string (it is the same as the original string),
    # and fall back to Google in this case.
    original_input = translations.get(source_language) or translations.get("en")
    for lang, value in google_translations.items():
        if lang in translations:
            if original_input and translations[lang] == original_input:
                translations[lang] = value
        else:
            translations[lang] = value

    # Remove duplicates for regional variations.
    for regional in ["en-GB", "pt-BR", "es-419"]:
        main = regional.split("-")[0]  # 'en', 'pt', 'es', ...
        if regional in translations and main in translations:
            if translations[regional] == translations[main]:
                translations.pop(regional)

    print("\nMerged Deepl and Google translations:")
    en = translations.pop("en")
    langs = list(translations.keys())
    langs.sort()

    categories_txt_languages = get_supported_categories_txt_languages()
    absent_in_categories_txt = [
        item for item in langs if item not in categories_txt_languages
    ]
    print("============ categories.txt format ============")
    if len(absent_in_categories_txt) > 0:
        print(
            "\nWARNING: The following translations are not supported yet in the categories.txt and are skipped:"
        )
        print(absent_in_categories_txt)
        print("See indexer/categories_holder.hpp for details.\n")
    print("en:" + en)
    for lang in langs:
        if lang in absent_in_categories_txt:
            continue
        print(lang + ":" + translations[lang])

    print("\n============ strings.txt format ============")
    print("    en =", en)
    for lang in langs:
        print("   ", lang, "=", translations[lang])


if __name__ == "__main__":
    text_to_translate, context = parse_args(sys.argv[0])

    if not "DEEPL_FREE_API_KEY" in os.environ and not "DEEPL_API_KEY" in os.environ:
        print(
            "Error: neither DEEPL_FREE_API_KEY nor DEEPL_API_KEY environment variables are set."
        )
        print(
            "DeepL translations are not available. Register for a free Developer API account here:"
        )
        print("https://www.deepl.com/pro#developer")
        print("and get the API key here: https://www.deepl.com/account/summary")
        exit(1)

    if shutil.which(TRANS_CMD) is None:
        print("Error: translate-shell program for Google Translate is not installed.")
        if platform.system() == "Darwin":
            print("Install it using `brew install translate-shell`")
        else:
            print(
                "See https://github.com/soimort/translate-shell/wiki/Distros for installation instructions."
            )
        exit(1)
    main(text_to_translate, context)
