#!/bin/bash
# Should be used everywhere to generate a consistent version number based
# on the date of the last commit and a number of commits on that day.
set -euo pipefail


function init {
  if ! type git >/dev/null 2>&1 || ! git rev-parse --is-inside-work-tree >/dev/null 2>&1 ; then
    # Either git is not installed on the system or this is not a git work tree.
    # Likely because the code is built from an archive.
    # In any case determining commit count, date and hash won't be possible.
    DATE=$(TZ=UTC0 date +%Y.%m.%d)
    COUNT=0
    GIT_HASH="00000000"
  else
    # Use HEAD's UTC commit day and count all reachable commits on that day.
    # Commit dates need not be ordered, so an older date cannot terminate the scan.
    DATE=$(TZ=UTC0 git log -1 --format=%cd --date=format-local:%Y.%m.%d)
    COUNT=$(TZ=UTC0 git log --format=%cd --date=format-local:%Y.%m.%d \
      | awk -v date="$DATE" '$0 == date { count++ } END { print count + 0 }')
    GIT_HASH=$(git describe --match="" --always --abbrev=8 --dirty)
  fi
}

function ios_version {
  echo "$DATE"
}

function ios_build {
  echo "$COUNT"
}

function count {
  echo "$COUNT"
}

function  android_name {
  echo "$DATE-$COUNT"
}

function android_code {
  # yy_MM_dd_CCC; the leading billion is reserved for the Wear artifact.
  # yy - year
  # MM - month
  # dd - day
  # CCC - fixed-width count of commits on HEAD's UTC day.
  # Phone and Wear codes must both fit Google Play's limit of 2,100,000,000.
  # See https://developer.android.com/studio/publish/versioning for details.
  local cutYear=${DATE:2}
  if (( COUNT > 999 )); then
    echo "Android version code supports at most 999 commits per UTC day" >&2
    return 1
  fi
  echo "${cutYear//./}$(printf %03d "$COUNT")"
}

function qt_int_version {
  # yy_MM_dd
  # yy - year
  # MM - month
  # dd - day
  local cutYear=${DATE:2}
  echo "${cutYear//./}"
}

function windows_version {
  # Windows package versions have four numeric components; Store reserves the last one.
  local month_day="${DATE:5:2}${DATE:8:2}"
  echo "${DATE:0:4}.$((10#$month_day)).$COUNT.0"
}

function qt_version {
  local OS_NAME=$(uname -s)
  echo "$DATE-$COUNT-$GIT_HASH-$OS_NAME"
}

function usage {
  cat << EOF
Prints Organic Maps version in specified format.
Version is the last git commit's date plus a number of commits on that day.
Usage: $0 <format>
Where format is one of the following arguments (shows current values):
  ios_version    $(ios_version)
  ios_build      $(ios_build)
  android_name   $(android_name)
  android_code   $(android_code)
  qt_version     $(qt_version)
  qt_int_version $(qt_int_version)
  windows_version $(windows_version)
  count          $(count)
EOF
}

init

if [ -z ${1:-} ] || [[ ! $(type -t "$1") == function ]]; then
  usage
  exit 1
else
  "$1"
fi
