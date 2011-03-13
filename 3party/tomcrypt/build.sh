#!/bin/bash
echo "$1 ($2, $3)..."
make clean 1>/dev/null 2>/dev/null
echo -n "building..."
CFLAGS="$2 $CFLAGS $4" EXTRALIBS="$5" make -j4 -f $3 test tv_gen 1>gcc_1.txt 2>gcc_2.txt || (echo "build $1 failed see gcc_2.txt for more information" && cat gcc_2.txt && exit 1)
echo -n "testing..."
if [ -a test ] && [ -f test ] && [ -x test ]; then
   ((./test >test_std.txt 2>test_err.txt && ./tv_gen > tv.txt) && echo "$1 test passed." && echo "y" > testok.txt) || (echo "$1 test failed" && cat test_err.txt && exit 1)
   if find *_tv.txt -type f 1>/dev/null 2>/dev/null ; then
      for f in *_tv.txt; do if (diff --ignore-case $f notes/$f) then true; else (echo "tv_gen $f failed" && rm -f testok.txt && exit 1); fi; done
   fi
fi
if [ -a testok.txt ] && [ -f testok.txt ]; then
   exit 0
fi
exit 1

# $Source: /cvs/libtom/libtomcrypt/build.sh,v $   
# $Revision: 1.9 $   
# $Date: 2006/03/18 14:10:55 $ 
