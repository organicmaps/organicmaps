#!/usr/bin/env bash
# Shared configuration for code formatting scripts.
# Sourced by both tools/unix/clang-format.sh and tools/hooks/pre-commit.
#
# Each entry is "directory|find_name_pattern".
# Patterns use find's -name syntax; multiple patterns are OR'd with -o.

CLANG_FORMAT_TARGETS=(
  # Android – Java
  "android/app/src|*.java"
  "android/libs/api/src|*.java"
  "android/libs/branding/src|*.java"
  "android/libs/car/src|*.java"
  "android/libs/downloader/src|*.java"
  "android/libs/googleassistant/src|*.java"
  "android/libs/routing/src|*.java"
  "android/libs/utils/src|*.java"
  "android/sdk/car/src|*.java"
  "android/sdk/src|*.java"
  "android/sdk/widgets/lanes/src|*.java"
  "android/sdk/widgets/speedlimit/src|*.java"
  # Android – C++
  "android/sdk/src/main/cpp|*.[hc]pp"
  # iOS – C++/ObjC
  "iphone|*.[hc]pp"
  "iphone|*.[hm]"
  "iphone|*.mm"
  # Core / C++
  "dev_sandbox|*.[hc]pp"
  "generator|*.[hc]pp"
  "libs|*.[hc]pp"
  "qt|*.[hc]pp"
  "tools|*.[hc]pp"
)

SWIFTFORMAT_TARGETS=(
  "iphone|*.swift"
)

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

# Build a grep -E pattern that matches any file covered by a targets array.
# Usage: build_path_regex CLANG_FORMAT_TARGETS[@]
build_path_regex() {
  local _targets_name="$1"
  eval "local _entries=(\"\${${_targets_name}[@]}\")"
  local parts=()
  for entry in "${_entries[@]}"; do
    local dir="${entry%%|*}"
    local glob="${entry##*|}"
    # Convert find glob to regex: *.[hc]pp -> \.[hc]pp$, *.java -> \.java$
    local ext_re
    ext_re=$(printf '%s' "$glob" | sed 's/\*//; s/\./\\./g')
    # Escape directory slashes for regex
    local dir_re
    dir_re=$(printf '%s' "$dir" | sed 's/\//\\\//g')
    parts+=("^${dir_re}/.*${ext_re}\$")
  done
  # Join with |
  local IFS='|'
  echo "${parts[*]}"
}
