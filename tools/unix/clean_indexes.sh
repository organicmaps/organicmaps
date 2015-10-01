#!/bin/bash
# Removes indices after an app run. You can pass a path as the first argument.
set -u -x
TARGET="${1:-$(dirname "$0")/../../data}"
for mwm in "$TARGET/"*.mwm; do
  rm -rf "$TARGET/$(basename "$mwm" .mwm)"
done
