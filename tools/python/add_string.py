#!/usr/bin/env python3

"""
Script to automate adding new translation strings to strings.txt.

Usage:
    ./add_string.py --key "string_key" --en "English text" --comment "Description" --tags "android-app"

Requirements:
    - DEEPL_FREE_API_KEY or DEEPL_API_KEY environment variable
    - GEMINI_API_KEY environment variable (for proofreading)
    - translate-shell installed (brew install translate-shell)
"""

import argparse
import json
import os
import requests
import sys
from pathlib import Path
from typing import Dict, Optional

# Import translation functions from translate.py
from translate import (
    GOOGLE_ONLY_LANGUAGES,
    deepl_translate,
    google_translate,
)


def get_gemini_api_key() -> Optional[str]:
    """Get Gemini API key from environment."""
    return os.environ.get("GEMINI_API_KEY")


def proofread_with_gemini(
        translations: Dict[str, str],
        english_text: str,
        comment: str,
) -> Dict[str, str]:
    """
    Send translations to Gemini for proofreading and improvements.
    Returns a dict with only the languages that need changes.
    """
    api_key = get_gemini_api_key()
    if not api_key:
        print("Warning: GEMINI_API_KEY not set, skipping proofreading")
        return {}

    # Prepare translations as JSON for the prompt
    translations_json = json.dumps(translations, ensure_ascii=False, indent=2)

    prompt = f"""You are a professional translator reviewer for a mobile maps app (Organic Maps).
Context: {comment}
English text: {english_text}

Here are the translations to review:
{translations_json}

Review and improve these translations. Consider:
- UI context (button labels should be short, max 1-2 words if the English is short)
- Consistency with app terminology
- Natural phrasing in each language
- Grammar and spelling correctness

Return ONLY a valid JSON object with improved translations.
Only include languages that need changes (don't include languages that are already good).
If all translations are good, return an empty object: {{}}

Example response format:
{{"ru": "improved Russian text", "de": "improved German text"}}
"""

    url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key={api_key}"
    headers = {"Content-Type": "application/json"}
    payload = {
        "contents": [{"parts": [{"text": prompt}]}],
        "generationConfig": {
            "temperature": 0.1,
            "topP": 0.95,
            "topK": 40,
            "maxOutputTokens": 8192,
        },
    }

    try:
        response = requests.post(url, headers=headers, json=payload, timeout=60)
        response.raise_for_status()
        result = response.json()

        # Extract the text response
        text = result["candidates"][0]["content"]["parts"][0]["text"]

        # Parse JSON from response (handle markdown code blocks)
        text = text.strip()
        if text.startswith("```json"):
            text = text[7:]
        if text.startswith("```"):
            text = text[3:]
        if text.endswith("```"):
            text = text[:-3]
        text = text.strip()

        improvements = json.loads(text)
        if improvements:
            print(f"\nGemini suggested improvements for {len(improvements)} language(s):")
            for lang, improved_text in improvements.items():
                if lang in translations:
                    print(f"  {lang}: '{translations[lang]}' -> '{improved_text}'")
        return improvements

    except requests.exceptions.RequestException as e:
        print(f"Warning: Gemini API request failed: {e}")
        return {}
    except (json.JSONDecodeError, KeyError, IndexError) as e:
        print(f"Warning: Failed to parse Gemini response: {e}")
        return {}


def remove_regional_duplicates(translations: Dict[str, str]) -> Dict[str, str]:
    """
    Remove regional variations if they match the base language.
    e.g., if es-MX == es, remove es-MX
    """
    regional_to_base = {
        "es-MX": "es",
        "pt-BR": "pt",
        "en-GB": "en",
    }

    result = translations.copy()
    removed = []

    for regional, base in regional_to_base.items():
        if regional in result and base in result:
            if result[regional] == result[base]:
                del result[regional]
                removed.append(regional)

    if removed:
        print(f"\nRemoved duplicate regional variations: {', '.join(removed)}")

    return result


def format_string_block(
        key: str,
        comment: str,
        tags: str,
        translations: Dict[str, str],
) -> str:
    """Format a string block for strings.txt."""
    lines = [f"\n[{key}]"]
    lines.append(f"comment = {comment}")
    lines.append(f"tags = {tags}")

    # Sort languages, but put 'en' first
    langs = sorted(translations.keys())
    if "en" in langs:
        langs.remove("en")
        langs.insert(0, "en")

    for lang in langs:
        lines.append(f"{lang} = {translations[lang]}")

    return "\n".join(lines) + "\n"


def append_to_strings_txt(block: str) -> Path:
    """Append the string block to strings.txt."""
    script_path = Path(os.path.realpath(__file__))
    repo_dir = script_path.parent.parent.parent
    strings_txt_path = repo_dir / "data" / "strings" / "strings.txt"

    with open(strings_txt_path, "a", encoding="utf-8") as f:
        f.write(block)

    return strings_txt_path


def main():
    parser = argparse.ArgumentParser(
        description="Add a new translation string to strings.txt",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Example:
    ./add_string.py --key "save_button" --en "Save" --comment "Button to save changes" --tags "android-app,apple-maps"
""",
    )
    parser.add_argument("--key", required=True, help="String key (e.g., button_save)")
    parser.add_argument("--en", required=True, help="English text")
    parser.add_argument("--comment", required=True, help="Comment for translators")
    parser.add_argument("--tags", required=True,
                        help="Platform tags (e.g., android-app,apple-maps)")

    args = parser.parse_args()

    # Validate environment
    if "DEEPL_FREE_API_KEY" not in os.environ and "DEEPL_API_KEY" not in os.environ:
        print("Error: DEEPL_FREE_API_KEY or DEEPL_API_KEY environment variable is required")
        sys.exit(1)

    print(f"Adding string '{args.key}' with English text: \"{args.en}\"")
    print(f"Comment: {args.comment}")
    print(f"Tags: {args.tags}")
    print()

    # Step 1: Get translations via DeepL
    translations = deepl_translate(args.en, "en", context=args.comment)

    # Step 2: Get Google translations for languages not supported by DeepL
    if GOOGLE_ONLY_LANGUAGES:
        google_translations = google_translate(args.en, "en")
        translations.update(google_translations)

    # Step 3: Proofread with Gemini
    improvements = proofread_with_gemini(translations, args.en, args.comment)
    translations.update(improvements)

    # Step 4: Remove regional duplicates
    translations = remove_regional_duplicates(translations)

    # Ensure English is included
    translations["en"] = args.en

    # Step 5: Format and write to strings.txt
    block = format_string_block(args.key, args.comment, args.tags, translations)

    print("\n============ Adding to strings.txt ============")
    print(block)

    strings_path = append_to_strings_txt(block)
    print(f"Successfully added to {strings_path}")


if __name__ == "__main__":
    main()
