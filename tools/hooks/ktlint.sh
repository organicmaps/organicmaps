#!/usr/bin/env bash

if command -v ktlint >/dev/null 2>&1; then
  ktlint $@
else
  echo "ktlint not found. Please install ktlint to format Kotlin files."
fi
