#!/usr/bin/env bash
# Shared configuration for code formatting scripts.
# Sourced by tools/unix/clang-format.sh, tools/unix/ktlint_format.sh and tools/hooks/pre-commit.
#
# Targets are git pathspecs matched by extension, not by directory: per-module
# directory lists silently drift when modules are added, split or moved.
# Files that must not be reformatted are listed in .clang-format-ignore,
# which clang-format applies on its own.

CLANG_FORMAT_TARGETS=(
  '*.java'
  '*.[hc]pp'
  '*.[hm]'  # Legacy C++ headers and Objective-C implementations.
  '*.mm'
)

SWIFTFORMAT_TARGETS=(
  "iphone"
)

KTLINT_TARGETS=(
  'android/*.kt'
)

resolve_ktlint() {
  if command -v ktlint >/dev/null 2>&1; then
    echo "ktlint"
    return 0
  fi
  return 1
}

# Resolve a clang-format binary, preferring a versioned one.
resolve_clang_format() {
  local binary
  for binary in clang-format clang-format-22; do
    if command -v "$binary" >/dev/null 2>&1; then
      echo "$binary"
      return 0
    fi
  done
  echo "Error: clang-format not found." >&2
  return 1
}
