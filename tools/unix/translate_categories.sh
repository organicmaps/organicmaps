#!/usr/bin/env bash

# Install translate-shell before using this script:
# https://github.com/soimort/translate-shell
# Use `brew install translate-shell` on Mac OS X.

# There is a rate-limit for Google which can be work-arounded by using
# another IP or IPv6.

set -euo pipefail

DELIM=${DELIM:-:}

case $# in
  1) SRC=en
     WORD="$1"
     ;;
  2) SRC="$1"
     WORD="$2"
     ;;
  *) echo "Usage: [DELIM=' = '] $0 word_or_text_in_English"
     echo "       or"
     echo "       [DELIM=' = '] $0 source_language_code word_or_text_in_given_language"
     exit 1
     ;;
esac

# Note: default Google engine doesn't properly support European Portuguese (pt-PT)
# and always produces Brazilian translations. Need to use Deepl.
LANGUAGES=( en ar be bg ca cs da de el es et eu fa 'fi' fr he hu id it ja ko mr nb nl pl pt pt-BR ro ru sk sv sw th tr uk vi zh-CN zh-TW )

for lang in "${LANGUAGES[@]}"; do
  TRANSLATION=$(trans -b "$SRC:$lang" "$WORD" | sed 's/   *//')
  # Correct language codes to ours.
  case $lang in
    zh-CN) lang="zh-Hans" ;;
    zh-TW) lang="zh-Hant" ;;
    pt-PT) lang="pt"      ;;
  esac
  echo "$lang${DELIM}$(tr '[:lower:]' '[:upper:]' <<< "${TRANSLATION:0:1}")${TRANSLATION:1}"
  # To avoid quota limits.
  sleep 0.5
done
