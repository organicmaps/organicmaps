#!/bin/bash
#####################################################
# Tests a planet build made with generate_planet.sh #
#####################################################

if [ $# -eq 0 ]; then
  echo
  echo "This script analyzes a generate_planet.sh run and prints all issues."
  echo "Usage: $0 <target_dir> [<old_maps_dir>]"
  echo
  exit 1
fi

set -u # Fail on undefined variables

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
TARGET="${TARGET:-$1}"
LOG_PATH="${LOG_PATH:-$TARGET/logs}"
PLANET_LOG="$LOG_PATH/generate_planet.log"
DELTA_WITH=
[ $# -gt 1 -a -d "${2-}" ] && DELTA_WITH="$2"

source "$(dirname "$0")/find_generator_tool.sh"

# Step 1: analyze logs and find errors
echo
echo '### LOGS'
grep -i 'error\|warn\|critical\|fail\|abort\|останов\|fatal' "$PLANET_LOG" | grep -v 'settings\.ini'
for log in "$LOG_PATH"/*.log; do
  if [ "$log" != "$PLANET_LOG" -a "$log" != "$LOG_PATH/test_planet.log" ]; then
    CONTENT="$(grep -i 'error\|warn\|critical\|fail\|abort\|останов\|fatal\|fault' "$log" | grep -v 'settings\.ini\|language file for co\|Zero length lin\|too many tokens\|Equal choices for way\|No feature id for way\|number of threads is\|Invalid order of edges')"
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
if [ -n "$(ls "$TARGET" | grep '\.mwm\.routing')" ]; then
  # Only display missing routing files for existing MWMs
  for mwm in "$TARGET"/*.mwm; do
    if [[ "$mwm" != *World* ]]; then
      [ ! -f "$mwm.routing" ] && echo "$(basename "$mwm").routing"
    fi
  done
fi

# Step 2.5: compare new files sizes with old
if [ -n "$DELTA_WITH" ]; then
  echo
  echo "### SIZE DIFFERENCE WITH $DELTA_WITH"
  python "$(dirname "$0")/diff_size.py" "$TARGET" "$DELTA_WITH" 5
fi

# For generator_tool, we create a temporary directory with symlinks to all maps
# That way, it can be easily cleaned after routing engine creates a lot of temporary directories in it
FTARGET="$TARGET/symlinked_copy"
mkdir -p "$FTARGET"
TARGET_PATH="$(cd "$TARGET"; pwd)"
for file in "$TARGET"/*.mwm*; do
  BASENAME="$(basename "$file")"
  ln -s "$TARGET_PATH/$BASENAME" "$FTARGET/$BASENAME"
done

# Step 3: run calc_statistics and check for sections
echo
echo '### MISSING MWM SECTIONS'
FOUND_COASTS=
for mwm in "$FTARGET"/*.mwm; do
  BASENAME="$(basename "$mwm" .mwm)"
  STAT="$("$GENERATOR_TOOL" --data_path="$FTARGET" --user_resource_path="$OMIM_PATH/data/" --output="$BASENAME" --calc_statistics 2>/dev/null)"
  [ -z "$FOUND_COASTS" -a -n "$(echo "$STAT" | grep 'natural|coastline|')" ] && FOUND_COASTS=1
  SECTIONS="$(echo "$STAT" | grep 'version : 8' | wc -l |  tr -d ' ')"
  [ -f "$mwm.routing" -a "$SECTIONS" != "2" ] && echo "$BASENAME: $SECTIONS"
done

[ -z "$FOUND_COASTS" ] && echo && echo 'WARNING: Did not find any coastlines in MWM files'

# Step 3.5: run type_statistics for old and new files to compare
if [ -n "$DELTA_WITH" ]; then
  echo
  echo '### FEATURE DIFFERENCE'
  TMPBASE="$HOME/test_planet_tmp"
  for mwm in "$FTARGET"/*.mwm; do
    BASENAME="$(basename "$mwm" .mwm)"
    if [ -f "$DELTA_WITH/$BASENAME.mwm" ]; then
      "$GENERATOR_TOOL" --data_path="$FTARGET"    --user_resource_path="$OMIM_PATH/data/" --output="$BASENAME" --type_statistics >"${TMPBASE}_new" 2>/dev/null
      "$GENERATOR_TOOL" --data_path="$DELTA_WITH" --user_resource_path="$OMIM_PATH/data/" --output="$BASENAME" --type_statistics >"${TMPBASE}_old" 2>/dev/null
      DIFFERENCE="$(python "$(dirname "$0")/diff_features.py" "${TMPBASE}_new" "${TMPBASE}_old" 50)"
      if [ -n "$DIFFERENCE" ]; then
        echo
        echo "$BASENAME"
        echo "$DIFFERENCE"
      fi
    fi
  done
  rm "$TMPBASE"_*
fi

# Step 4: run intergation tests
echo
echo '### INTEGRATION TESTS'
"$(dirname "$GENERATOR_TOOL")/routing_integration_tests" "--data_path=$FTARGET/" "--user_resource_path=$OMIM_PATH/data/" "--suppress=online_cross_tests.*" 2>&1

# Step 5: run consistency tests
echo
echo '### CONSISTENCY TEST'
"$(dirname "$GENERATOR_TOOL")/routing_consistency_test" "--data_path=$FTARGET/" "--user_resource_path=$OMIM_PATH/data/" "--input_file=$OMIM_PATH/data/routing_statistics.log" 2>&1

# Clean the temporary directory
rm -r "$FTARGET"
