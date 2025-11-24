#!/usr/bin/env bash

set -euxo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd "$SCRIPT_DIR"

if [[ $OSTYPE == 'darwin'* ]]; then
  brew install --cask fontforge
  /Applications/FontForge.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 generate_font.py
else
  # Ubuntu/Debian
  sudo apt install python3-fontforge
  python3 generate_font.py
fi
