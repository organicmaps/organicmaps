#!/usr/bin/env bash

set -euo pipefail

EXCLUDE="\[tools\]|\[string\]\[generator\]"

case "${1:-default}" in
  firebase)
    LAST_TAG=$(git describe --tags --abbrev=0)
    git log --pretty='- %s' $LAST_TAG..HEAD|
      grep -ivE "\[ios\]|$EXCLUDE"|
      tail -n 199 # 16k chars max, one line is 80 chars = 200 lines max
    ;;
  testflight)
    LAST_TAG=$(git describe --tags --abbrev=0)
    git log --pretty='- %s' $LAST_TAG..HEAD|
      grep -ivE "\[android\]|$EXCLUDE"|
      tail -n 49 # 4000 chars max, one line is 80 chars = 50 lines max
    ;;
  *)
    >&2 echo "Usage: $0 firebase|gplay|testflight"
    exit 1
  ;;
esac
