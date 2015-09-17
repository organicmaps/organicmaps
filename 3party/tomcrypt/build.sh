#!/bin/bash
echo "$1 ($2, $3)..."

make clean 1>/dev/null 2>/dev/null

echo -n "building..."

CFLAGS="$2 $CFLAGS $4" EXTRALIBS="$5" make -f $3 test tv_gen 1>gcc_1.txt 2>gcc_2.txt
mret=$?
cnt=$(wc -l < gcc_2.txt)
# ignore 2 lines since ar prints to stderr instead of stdout and ar is called for 
# $(LIBNAME) and testprof/$(LIBTEST_S)
if [[ $mret -ne 0 ]] || [[ $cnt -gt 2 ]]; then
   echo "build $1 failed! printing gcc_2.txt now for convenience"
   cat gcc_2.txt
   exit 1
fi

echo -n "testing..."

if [ -a test ] && [ -f test ] && [ -x test ]; then
   ((./test >test_std.txt 2>test_err.txt && ./tv_gen > tv.txt) && echo "$1 test passed." && echo "y" > testok.txt) || (echo "$1 test failed, look at test_err.txt" && exit 1)
   if find *_tv.txt -type f 1>/dev/null 2>/dev/null ; then
      for f in *_tv.txt; do if (diff -i -w -B $f notes/$f) then true; else (echo "tv_gen $f failed" && rm -f testok.txt && exit 1); fi; done
   fi
fi

lcov_opts="--capture --no-external --directory src -q"
lcov_out=$(echo coverage_$1_$2_$3 | tr ' -=+' '_')".info"

if [ -a testok.txt ] && [ -f testok.txt ]; then
   [ "$LTC_COVERAGE" != "" ] && lcov $lcov_opts --output-file $lcov_out
   exit 0
fi
exit 1

# $Source$
# $Revision$
# $Date$
