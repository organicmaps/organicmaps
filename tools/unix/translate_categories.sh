#!/bin/bash

set -e -u

case $# in
  1) SRC=en
     WORD="$1"
     ;;
  2) SRC="$1"
     WORD="$2"
     ;;
  *) echo "Usage: $0 word_in_English"
     echo "       or"
     echo "       $0 language_code word_in_given_language"
     exit 1
     ;;
esac

LANGUAGES=( en ar cs da de el es fi fr he hu id it ja ko nb nl pl pt ro ru sk sv sw th tr uk vi zh-CN zh-TW )

for lang in "${LANGUAGES[@]}"; do
  TRANSLATION=$(trans -b "$SRC:$lang" "$WORD" | sed 's/   *//')
  # Use our categories.txt names for Chinese.
  if [[ $lang == "zh-CN" ]]; then
    lang="zh-Hans";
  elif [[ $lang == "zh-TW" ]]; then
    lang="zh-Hant";
  fi
  echo "$lang:$(tr '[:lower:]' '[:upper:]' <<< ${TRANSLATION:0:1})${TRANSLATION:1}"
done
