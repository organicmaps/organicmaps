#!/usr/bin/env python3

# Requires `brew install translate-shell`
# and DEEPL_FREE_API_KEY or DEEPL_API_KEY environment variables set.

import os
import subprocess
import requests
import json
import sys

# Use DeepL when possible with a fall back to Google.
GOOGLE_TARGET_LANGUAGES = [
  'ar',
  'be',
  'ca',
  'es-MX',
  'eu',
  'fa',
  'he',
  'mr',
  'th',
  'vi',
  'zh-TW',  # zh-Hant in OM
]

DEEPL_TARGET_LANGUAGES = [
    'bg',
    'cs',
    'da',
    'de',
    'el',
    'en-GB',
    'en-US', # en in OM
    'es',
    'et',
    'fi',
    'fr',
    'hu',
    'id',
    'it',
    'ja',
    'ko',
#    'lt',
#    'lv',
    'nb',
    'nl',
    'pl',
    'pt-BR',
    'pt-PT',  # pt in OM
    'ro',
    'ru',
    'sk',
#    'sl',
    'sv',
    'tr',
    'uk',
    'zh',  # zh-Hans in OM
]


def get_api_key():
  key = os.environ.get('DEEPL_FREE_API_KEY')
  if key == None:
    key = os.environ.get('DEEPL_API_KEY')
  if key == None:
    print('Error: DEEPL_FREE_API_KEY or DEEPL_API_KEY env variables are not set')
    exit(1)
  return key

def google_translate(text):
  fromTo = 'en:'
  for lang in GOOGLE_TARGET_LANGUAGES:
    fromTo += lang + '+'
  # Remove last +
  fromTo = fromTo[:-1]
  res = subprocess.run(['trans', '-b', '-no-bidi', fromTo, text], text=True, capture_output=True)
  if res.returncode != 0:
    print('Error running trans program:')
    print(res.stderr)
    exit(1)

  print('\nGoogle translations:')
  translations = {}
  i = 0
  for line in res.stdout.splitlines():
    lang = GOOGLE_TARGET_LANGUAGES[i]
    if lang == 'zh-TW':
      lang = 'zh-Hant'
    translations[lang] = line
    i = i + 1
    print(lang + ' = ' + line)
  return translations

def deepl_translate_one(text, target_language):
  url = 'https://api-free.deepl.com/v2/translate'
  payload = {
      'auth_key': get_api_key(),
      'text': text,
      'target_lang': target_language,
  }
  headers = {'Content-Type': 'application/x-www-form-urlencoded'}
  response = requests.request('POST', url, headers=headers, data=payload)
  return response.json()['translations'][0]['text']

def deepl_translate(text):
  translations = {}
  print('Deepl translations:')
  for lang in DEEPL_TARGET_LANGUAGES:
    translation = deepl_translate_one(text, lang)
    if lang == 'pt-PT':
      lang = 'pt'
    elif lang == 'zh':
      lang = 'zh-Hans'
    elif lang == 'en-US':
      lang = 'en'
    translations[lang] = translation
    print(lang + ' = ' + translation)
  return translations

if __name__ == '__main__':
  text_to_translate = sys.argv[1]

  translations = deepl_translate(text_to_translate)
  google_translations = google_translate(text_to_translate)
  translations.update(google_translations)
  # Remove duplicates for regional variations.
  for regional in ['en-GB', 'es-MX', 'pt-BR']:
    main = regional.split('-')[0]  # 'en', 'es', 'pt'...
    if translations[regional] == translations[main]:
      translations.pop(regional)

  print('\nMerged Deepl and Google translations:')
  en = translations.pop('en')
  langs = list(translations.keys())
  langs.sort()
  print('en =', en)
  for lang in langs:
    print(lang, '=', translations[lang])
