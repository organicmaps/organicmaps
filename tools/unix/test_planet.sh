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

SCRIPT_PATH="$(dirname "$0")"
OMIM_PATH="${OMIM_PATH:-$(cd "$SCRIPT_PATH/../.."; pwd)}"
TARGET="$(cd "${TARGET:-$1}"; pwd)"
LOG_PATH="${LOG_PATH:-$TARGET/logs}"
PLANET_LOG="$LOG_PATH/generate_planet.log"
DELTA_WITH=
BOOKING_THRESHOLD=20
[ $# -gt 1 -a -d "${2-}" ] && DELTA_WITH="$2"

source "$SCRIPT_PATH/find_generator_tool.sh"

# Step 1: analyze logs and find errors
echo
echo '### LOGS'
grep -i 'error\|warn\|critical\|fail\|abort\|останов\|fatal' "$PLANET_LOG" | grep -v 'settings\.ini'
for log in "$LOG_PATH"/*.log; do
  if [ "$log" != "$PLANET_LOG" -a "$log" != "$LOG_PATH/test_planet.log" ]; then
    CONTENT="$(grep -i 'error\|warn\|critical\|fail\|abort\|останов\|fatal\|fault' "$log" | \
      grep -v 'settings\.ini\|language file for co\|Zero length lin\|too many tokens\|Equal choices for way\|No feature id for way\|number of threads is\|Invalid order of edges')"
    if [ -n "$CONTENT" ]; then
      echo
      echo "$log"
      echo "$CONTENT"
    fi
  fi
done

# Step 2.1: test if mwms and routing were made
echo
echo '### MISSING FILES'
# Missing MWM files can be derived only from intermediate borders
if [ -d "$TARGET/borders" ]; then
  for border in "$TARGET/borders"/*.poly; do
    MWM="$(basename "$border" .poly).mwm"
    [ ! -f "$TARGET/$MWM" ] && echo "$MWM"
  done
fi

# Step 2.2: compare new files sizes with old
if [ -n "$DELTA_WITH" ]; then
  echo
  echo "### SIZE DIFFERENCE WITH $DELTA_WITH"
  python "$SCRIPT_PATH/diff_size.py" "$TARGET" "$DELTA_WITH" 5
  echo
  echo "Size of old data: $(ls -l "$DELTA_WITH"/*.mwm | awk '{ total += $5 }; END { print total/1024/1024/1024 }') GB"
  echo "Size of new data: $(ls -l     "$TARGET"/*.mwm | awk '{ total += $5 }; END { print total/1024/1024/1024 }') GB"
fi

# For generator_tool, we create a temporary directory with symlinks to all maps
# That way, it can be easily cleaned after routing engine creates a lot of temporary directories in it
FTARGET="$TARGET/symlinked_copy/$(basename "$TARGET")"
mkdir -p "$FTARGET"
for file in "$TARGET"/*.mwm*; do
  BASENAME="$(basename "$file")"
  ln -s "$TARGET/$BASENAME" "$FTARGET/$BASENAME"
done

# Step 3.1: run calc_statistics and check for sections
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

# Step 3.2: run type_statistics for old and new files to compare
if [ -n "$DELTA_WITH" ]; then
  echo
  echo '### FEATURE DIFFERENCE'
  TMPBASE="$HOME/test_planet_tmp"
  for mwm in "$FTARGET"/*.mwm; do
    BASENAME="$(basename "$mwm" .mwm)"
    if [ -f "$DELTA_WITH/$BASENAME.mwm" ]; then
      "$GENERATOR_TOOL" --data_path="$FTARGET"    --user_resource_path="$OMIM_PATH/data/" --output="$BASENAME" --type_statistics >"${TMPBASE}_new" 2>/dev/null
      "$GENERATOR_TOOL" --data_path="$DELTA_WITH" --user_resource_path="$OMIM_PATH/data/" --output="$BASENAME" --type_statistics >"${TMPBASE}_old" 2>/dev/null
      DIFFERENCE="$(python "$SCRIPT_PATH/diff_features.py" "${TMPBASE}_new" "${TMPBASE}_old" 50)"
      if [ -n "$DIFFERENCE" ]; then
        echo
        echo "$BASENAME"
        echo "$DIFFERENCE"
      fi
    fi
  done
  rm "$TMPBASE"_*
fi

# Step 3.3: check booking hotels count in new .mwm files
if [ -n "$DELTA_WITH" ]; then
  echo
  echo '### BOOKING HOTELS COUNT DIFFERENCE'
  python "$OMIM_PATH/tools/python/mwm/mwm_feature_compare.py" -n "$TARGET" -o "$DELTA_WITH" -f "sponsored-booking" -t $BOOKING_THRESHOLD
fi

# Step 4: run integration tests
echo
echo '### INTEGRATION TESTS'
"$(dirname "$GENERATOR_TOOL")/routing_integration_tests" "--data_path=$FTARGET/../" "--user_resource_path=$OMIM_PATH/data/" "--suppress=online_cross_tests.*" 2>&1

# Clean the temporary directory
rm -r "$FTARGET"
