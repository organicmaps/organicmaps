#!/bin/bash
set -e -u

# If CMAKE variable is set, use it
[ -n "${CMAKE-}" -a -x "${CMAKE-}" ] && return 0

# Find cmake, prefer cmake3
for name in cmake3 cmake; do
  if command -v "$name" > /dev/null; then
    CMAKE="$name"
    return 0
  fi
done

# GUI builds on macOS may omit these CMake locations from PATH.
if [[ "$(uname -s)" == Darwin ]]; then
  for cmake_path in /opt/homebrew/bin/cmake /usr/local/bin/cmake /Applications/CMake.app/Contents/bin/cmake; do
    if [[ -x "$cmake_path" ]]; then
      CMAKE="$cmake_path"
      return 0
    fi
  done
fi

echo 'Error: cmake was not found. Install CMake or set CMAKE to its executable path.' >&2
exit 1
