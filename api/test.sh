#!/bin/bash
set -e -u -x
clang src/c/api-client.c  tests/c/api-client-test.c -o /tmp/api-client-test
/tmp/api-client-test
