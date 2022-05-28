#!/bin/bash

set -euo pipefail

function usage {
  cat << EOF
Prints Organic Maps version in specified format.
Version is the last git commit's date plus a number of commits on that day.
Usage: $0 format
Where format is one of the following arguments:
  ios_version   2021.12.26
  ios_build     3
  android_name  2021.12.26-3
  android_code  21122603
  git_hash      abcdef01
EOF
}

# Note: other ways to get date use the "when commit was rebased" date.
# This approach counts a number of commits each day based on author's commit date.
COUNT_AND_DATE=( $(git log --date=short --pretty=format:%ad --date=format:'%Y.%m.%d' --since="30 days ago" | sort | uniq -c | tail -1) )
if [ -z "$COUNT_AND_DATE" ]; then
  # Fallback: use today's date if there were no commits since last month.
  COUNT_AND_DATE=( 0 $(date +%Y.%m.%d) )
fi
DATE=${COUNT_AND_DATE[1]}
COUNT=${COUNT_AND_DATE[0]}

case "${1:-}" in
  ios_version)
    echo $DATE
    ;;
  ios_build)
    echo $COUNT
    ;;
  android_name)
    echo $DATE-$COUNT
    ;;
  # RR_yy_MM_dd_CC
  # RR - reserved to identify special markets, max value is 21.
  # yy - year
  # MM - month
  # dd - day
  # CC - the number of commits from the current day
  # 21_00_00_00_00 is the the greatest value Google Play allows for versionCode.
  # See https://developer.android.com/studio/publish/versioning for details.
  android_code)
    CUT_YEAR=${DATE:2}
    echo ${CUT_YEAR//./}$(printf %02d $COUNT)
    ;;
  git_hash)
    git describe --match="" --always --abbrev=8 --dirty
    ;;
  *)
    usage
    exit 1
    ;;
esac
