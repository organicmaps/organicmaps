#!/bin/bash
#####################################################
# Tests a planet build made with generate_planet.sh #
#####################################################

if [ $# -eq 0 ]; then
  echo
  echo "This script analyzes a generate_planet.sh run and prints all issues."
  echo "Usage: $0 <target_dir>"
  echo
  exit 1
fi

set -u # Fail on undefined variables

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
TARGET="${TARGET:-$1}"
LOG_PATH="${LOG_PATH:-$TARGET/logs}"
PLANET_LOG="$LOG_PATH/generate_planet.log"

source "$(dirname "$0")/find_generator_tool.sh"

# Step 1: analyze logs and find errors
echo
echo '### LOGS'
grep -i 'error\|warn\|critical\|fail\|abort\|останов\|fatal' "$PLANET_LOG" | grep -v 'settings\.ini'
for log in "$LOG_PATH"/*.log; do
  if [ "$log" != "$PLANET_LOG" -a "$log" != "$LOG_PATH/test_planet.log" ]; then
    CONTENT="$(grep -i 'error\|warn\|critical\|fail\|abort\|останов\|fatal' "$log" | grep -v 'settings\.ini\|language file for co\|Zero length lin\|too many tokens\|Equal choices for way\|No feature id for way\|number of threads is')"
    if [ -n "$CONTENT" ]; then
      echo
      echo "$log"
      echo "$CONTENT"
    fi
  fi
done

# Step 2: test if mwms and routing were made
echo
echo '### MISSING FILES'
# Missing MWM files can be derived only from intermediate borders
if [ -d "$TARGET/borders" ]; then
  for border in "$TARGET/borders"/*.poly; do
    MWM="$(basename "$border" .poly).mwm"
    [ ! -f "$TARGET/$MWM" ] && echo "$MWM"
  done
fi
# Only display missing routing files for existing MWMs
for mwm in "$TARGET"/*.mwm; do
  if [[ "$mwm" != *World* ]]; then
    [ ! -f "$mwm.routing" ] && echo "$(basename "$mwm").routing"
  fi
done

# Step 3: run calc_statistics and check for sections
echo
echo '### MISSING MWM SECTIONS'
FOUND_COASTS=
for mwm in "$TARGET"/*.mwm; do
  BASENAME="$(basename "$mwm" .mwm)"
  STAT="$("$GENERATOR_TOOL" --data_path="$TARGET" --user_resource_path="$OMIM_PATH/data/" --output="$BASENAME" --calc_statistics 2>/dev/null)"
  [ -z "$FOUND_COASTS" -a -n "$(echo "$STAT" | grep 'natural|coastline|')" ] && FOUND_COASTS=1
  SECTIONS="$(echo "$STAT" | grep 'version : 8' | wc -l |  tr -d ' ')"
  [ -f "$mwm.routing" -a "$SECTIONS" != "2" ] && echo "$BASENAME: $SECTIONS"
done

[ -z "$FOUND_COASTS" ] && echo && echo 'WARNING: Did not find any coastlines in MWM files'

# Step 4: run intergation tests
echo
echo '### INTEGRATION TESTS'
# First, create a temporary directory with symlinks to all maps
# That way, it can be easily cleaned after routing engine creates a lot of temporary directories in it
FTARGET="$TARGET/symlinked_copy"
mkdir -p "$FTARGET"
TARGET_PATH="$(cd "$TARGET"; pwd)"
for file in "$TARGET"/*.mwm*; do
  BASENAME="$(basename "$file")"
  ln -s "$TARGET_PATH/$BASENAME" "$FTARGET/$BASENAME"
done
"$(dirname "$GENERATOR_TOOL")/integration_tests" "--data_path=$FTARGET/" "--user_resource_path=$OMIM_PATH/data/" "--suppress=online_cross_tests.*" 2>&1
rm -r "$FTARGET"
