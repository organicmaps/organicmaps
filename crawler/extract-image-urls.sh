#!/bin/bash
set -e -u -x

grep --ignore-case --only-matching --no-filename '<img[^/]*src=\"[^">]*"' *.opt \
  | sed 's/<img.*src="//g' \
  | sed 's/"$//g' \
  | sort -u \
  > $1
