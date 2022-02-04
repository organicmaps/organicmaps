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

DATE_OF_THE_LAST_COMMIT=$(git log -1 --date=format:%Y.%m.%d --pretty=format:%cd)
function commits_count {
  git rev-list --count --after="${DATE_OF_THE_LAST_COMMIT//./-}T00:00:00Z" HEAD
}

case "${1:-}" in
  ios_version)
    echo $DATE_OF_THE_LAST_COMMIT
    ;;
  ios_build)
    commits_count
    ;;
  android_name)
    echo $DATE_OF_THE_LAST_COMMIT-$(commits_count)
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
    CUT_YEAR=${DATE_OF_THE_LAST_COMMIT:2}
    echo ${CUT_YEAR//./}$(printf %02d $(commits_count))
    ;;
  git_hash)
    git describe --match="" --always --abbrev=8 --dirty
    ;;
  *)
    usage
    exit 1
    ;;
esac
