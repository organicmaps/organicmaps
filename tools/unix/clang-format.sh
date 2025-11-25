#!/usr/bin/env bash
set -euo pipefail

for binary in clang-format clang-format-21; do
  if command -v "$binary" >/dev/null 2>&1; then
    CLANG_FORMAT="$binary"
  fi
done

"$CLANG_FORMAT" --version

echo "Running clang-format on all repository files..."

XARGS_COMMAND="xargs -n1 -0 -P0 $CLANG_FORMAT -i"

# Android
find android/{app,sdk}/src -type f -name '*.java' -print0 | $XARGS_COMMAND
find android/sdk/src/main/cpp -type f -name '*.[hc]pp' -print0 | $XARGS_COMMAND

# iOS
find iphone -type f -name '*.[hc]pp' -o -name '*.[hm]' -o -name '*.mm' -print0 | $XARGS_COMMAND

# Core/C++
find dev_sandbox generator libs qt tools -type f -name '*.[hc]pp' -print0 | $XARGS_COMMAND

git diff --exit-code
