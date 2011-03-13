#!/bin/bash

bash genlist.sh > tmplist

perl filter.pl makefile tmplist
mv -f tmp.delme makefile

perl filter.pl makefile.icc tmplist
mv -f tmp.delme makefile.icc

perl filter.pl makefile.shared tmplist
mv -f tmp.delme makefile.shared

perl filter.pl makefile.unix tmplist
mv -f tmp.delme makefile.unix

perl filter.pl makefile.msvc tmplist
sed -e 's/\.o /.obj /g' < tmp.delme > makefile.msvc

rm -f tmplist
rm -f tmp.delme
