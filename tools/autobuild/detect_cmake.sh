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

echo 'Error: cmake is not installed.' >&2
exit 1
