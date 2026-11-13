#!/usr/bin/env bash
set -euo pipefail

# Concatenates Android release notes in all languages into a single output format
# suitable to upload to Google Play to update existing notes.

exec python3 "$(dirname "$0")/../python/google_play_release_notes.py" export "$@"
