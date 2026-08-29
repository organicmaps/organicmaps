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

# Major version of clang-format used by CI. Different majors reformat the same
# code differently, so local and CI runs must agree. Also read by the workflow.
CLANG_FORMAT_VERSION=23

# Resolve the clang-format binary matching CLANG_FORMAT_VERSION, preferring a versioned name.
resolve_clang_format() {
  local binary version found=
  for binary in "clang-format-$CLANG_FORMAT_VERSION" clang-format; do
    command -v "$binary" >/dev/null 2>&1 || continue
    version=$("$binary" --version)
    if [[ "$version" == *"version $CLANG_FORMAT_VERSION."* ]]; then
      echo "$binary"
      return 0
    fi
    found="${found:-$version}"
  done
  echo "Error: clang-format $CLANG_FORMAT_VERSION.x is required, found: ${found:-none}." >&2
  echo "See docs/CODE_STYLE_GUIDE.md for installation instructions." >&2
  return 1
}
