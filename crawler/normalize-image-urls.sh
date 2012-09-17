#!/bin/bash
set -e -u -x

cat $1 | sed 's:/thumb\(/.*\)/[0-9][0-9]*px-.*$:\1:' | sort -u > $2
