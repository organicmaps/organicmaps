#!/bin/bash

set -e -u

if [ $# -lt 1 ]; then
  echo "Usage:"
  echo "  $0 {debug|release}"
  exit 115
fi

set -x

# Ignore GUI tests as they need display connection and can't be tested in linux console
find "`dirname $0`/../../../omim-build-$1/out/$1" -name "*_tests" \( ! -iname "gui_tests" \)| \
    grep -v /tmp/ | \
    awk '{ print $0" &&" } END { print "echo REALLY ALL TESTS PASSED" }' | \
    sh -x
