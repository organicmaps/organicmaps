#!/usr/bin/env bash

set -euo pipefail

readonly SCRIPT_NAME=$(basename "$0")
readonly LOG=$(mktemp "/tmp/${SCRIPT_NAME}.XXXXXX")
readonly SMOKE_SUITE=(  \
  base_tests            \
  coding_tests          \
  generator_tests       \
  indexer_tests         \
  map_tests             \
  mwm_tests             \
  platform_tests        \
  routing_tests         \
  search_tests          \
)
BUILD_DIR=.
SUITE=full

log() {
  echo "$@" 2>&1 | tee -a "$LOG"
}

die() {
  log "$@"
  echo "Terminated. Log is written to $LOG"
  exit 1
}

usage() {
  log "Usage: $0 [options]"
  log "Options:"
  log "  -b    path to build directory, default: ."
  log "  -s    test suite, smoke or full, default: full"
  log "  -f    regular expression which is applied to all tests, default: .*"
  log "  -h    prints this help message"
  log ""
  log "Smoke test suite consists of:"
  for testName in "${SMOKE_SUITE[@]}"
  do
      log "  " "$testName"
  done
  exit 1
}

while [ $# -ne 0 ]
do
  case "$1" in
    -b) BUILD_DIR=${2?"Build directory is not set"}
        shift
        ;;
    -s) SUITE=${2?"Suite name is not set"}
        shift
        ;;
    -f) FILTER=${2?"Test filter regex is not set"}
        shift
        ;;
    -h) usage
        ;;
  esac
  shift
done

if [ ! -d "$BUILD_DIR" ]
then
  die "Build directory $BUILD_DIR does not exists"
fi

cd "$BUILD_DIR"

case "$SUITE" in
  smoke) TESTS=("${SMOKE_SUITE[@]}")
         ;;
   full) TESTS=($(find . -maxdepth 1 -name '*_tests'))
         ;;
      *) die "Unknown test suite: $SUITE"
         ;;
esac

EXIT_STATUS=0
for testBin in "${TESTS[@]}"
do
  if [ ! -x "$testBin" ]
  then
    die "Can't find test $testBin"
  fi

  log "Running $testBin..."
  if [ -z "${FILTER+undefined}" ]
  then
    ./$testBin 2>&1 | tee -a "$LOG"
    if [ ${PIPESTATUS[0]} -ne 0 ]
    then
      EXIT_STATUS=1
    fi
  else
    ./$testBin --filter="$FILTER" 2>&1 | tee -a "$LOG"
    if [ ${PIPESTATUS[0]} -ne 0 ]
    then
      EXIT_STATUS=1
    fi
  fi
done

if [ $EXIT_STATUS -eq 0 ]
then
  log "All tests passed, see log for details."
else
  log "Some of tests failed, see log for details."
fi
log "Log is written to: $LOG"
exit $EXIT_STATUS
