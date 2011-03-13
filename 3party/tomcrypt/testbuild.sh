#!/bin/bash
echo "$1 (Build Only, $2, $3)..."
make clean 1>/dev/null 2>/dev/null
echo -n "building..."
touch testok.txt
CFLAGS="$2 $CFLAGS $4" EXTRALIBS="$5" make -f $3 test tv_gen 1>gcc_1.txt 2>gcc_2.txt || (echo "build $1 failed see gcc_2.txt for more information" && cat gcc_2.txt && rm -f testok.txt && exit 1)
if find testok.txt -type f 1>/dev/null 2>/dev/null ; then
   echo "successful"
   exit 0
fi
exit 1
