#!/usr/bin/env bash
set -euo pipefail

check_dir=$(mktemp -d)
trap 'rm -rf "$check_dir"' EXIT
unzip -q "$1" 'jni/*/liborganicmaps.so' proguard.txt -d "$check_dir" || {
  echo '::error::AAR lacks SDK native libraries or consumer rules'
  exit 1
}
test -s "$check_dir/proguard.txt" || { echo '::error::Missing SDK consumer rules'; exit 1; }

for so in "$check_dir"/jni/*/liborganicmaps.so; do
  file "$so"
  file "$so" | grep -q ', stripped' || { echo "::error::$so is not stripped"; exit 1; }
done
