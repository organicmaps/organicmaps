#!/bin/bash

set -e -u

if [ $# -lt 1 ]; then
  echo "Usage:"
  echo "  $0 {debug|release}"
  exit 115
fi

set -x

find "`dirname $0`/../../../omim-build-$1/out/$1" -name "*_tests" | \
    grep -v /tmp/ | \
    awk '{ print $0" &&" } END { print "echo REALLY ALL TESTS PASSED" }' | \
    sh -x
