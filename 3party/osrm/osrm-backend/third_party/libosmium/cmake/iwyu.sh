#!/bin/sh
#
# This will run IWYU (Include What You Use) on includes files. The iwyu
# program isn't very reliable and crashes often, but is still useful.
#
# TODO: This script should be integrated with cmake in some way...
#

cmdline="iwyu -Xiwyu --mapping_file=osmium.imp -std=c++11 -I include"

log=build/iwyu.log

echo "INCLUDE WHAT YOU USE REPORT:" >$log

allok=yes

mkdir -p build/check_reports

for file in `find include/osmium -name \*.hpp`; do
    mkdir -p `dirname build/check_reports/$file`
    ifile="build/check_reports/${file%.hpp}.iwyu"
    $cmdline $file >$ifile 2>&1
    if grep -q 'has correct #includes/fwd-decls' ${ifile}; then
        echo "\n\033[1m\033[32m========\033[0m \033[1m${file}\033[0m" >>$log
        echo "[OK] ${file}"
    elif grep -q 'Assertion failed' ${ifile}; then
        echo "\n\033[1m======== ${file}\033[0m" >>$log
        echo "[--] ${file}"
        allok=no
    else
        echo "\n\033[1m\033[31m========\033[0m \033[1m${file}\033[0m" >>$log
        echo "[  ] ${file}"
        allok=no
    fi
    cat $ifile >>$log
done

if [ "$allok" = "yes" ]; then
    echo "All files OK"
else
    echo "There were errors"
fi

