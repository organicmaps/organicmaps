#!/bin/bash
set -e -u -x

grep --ignore-case --only-matching --no-filename --mmap '<img[^/]*src=\"[^">]*"' -r --include=*.opt . \
  | sed 's/<img.*src="//g' \
  | sed 's/"$//g' \
  | sort -u \
  > $1
