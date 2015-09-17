#!/bin/bash

if [ $# -lt 3 ]
then
  echo "usage is: ${0##*/} <which makefile and other make options> <additional defines> <path to math provider>"
  echo "e.g. \"${0##*/} \"makefile -j3\" \"-DUSE_LTM -DLTM_DESC -I/path/to/libtommath\" /path/to/libtommath/libtommath.a\""
  exit -1
fi

# date
echo "date="`date`

# stock build
bash run.sh "STOCK" " " "$1" "$2" "$3" || exit 1

# SMALL code
bash run.sh "SMALL" "-DLTC_SMALL_CODE" "$1" "$2" "$3" || exit 1

# NOTABLES
bash run.sh "NOTABLES" "-DLTC_NO_TABLES" "$1" "$2" "$3" || exit 1

# SMALL+NOTABLES
bash run.sh "SMALL+NOTABLES" "-DLTC_SMALL_CODE -DLTC_NO_TABLES" "$1" "$2" "$3" || exit 1

# CLEANSTACK
bash run.sh "CLEANSTACK" "-DLTC_CLEAN_STACK" "$1" "$2" "$3" || exit 1

# CLEANSTACK + SMALL
bash run.sh "CLEANSTACK+SMALL" "-DLTC_SMALL_CODE -DLTC_CLEAN_STACK" "$1" "$2" "$3" || exit 1

# CLEANSTACK + NOTABLES
bash run.sh "CLEANSTACK+NOTABLES" "-DLTC_NO_TABLES -DLTC_CLEAN_STACK" "$1" "$2" "$3" || exit 1

# CLEANSTACK + NOTABLES + SMALL
bash run.sh "CLEANSTACK+NOTABLES+SMALL" "-DLTC_NO_TABLES -DLTC_CLEAN_STACK -DLTC_SMALL_CODE" "$1" "$2" "$3" || exit 1

# NO_FAST
bash run.sh "NO_FAST" "-DLTC_NO_FAST" "$1" "$2" "$3" || exit 1

# NO_FAST + NOTABLES
bash run.sh "NO_FAST+NOTABLES" "-DLTC_NO_FAST -DLTC_NO_TABLES" "$1" "$2" "$3" || exit 1

# NO_ASM
bash run.sh "NO_ASM" "-DLTC_NO_ASM" "$1" "$2" "$3" || exit 1

# NO_TIMING_RESISTANCE
bash run.sh "NO_TIMING_RESISTANCE" "-DLTC_NO_ECC_TIMING_RESISTANT -DLTC_NO_RSA_BLINDING" "$1" "$2" "$3" || exit 1

# CLEANSTACK+NOTABLES+SMALL+NO_ASM+NO_TIMING_RESISTANCE
bash run.sh "CLEANSTACK+NOTABLES+SMALL+NO_ASM+NO_TIMING_RESISTANCE" "-DLTC_CLEAN_STACK -DLTC_NO_TABLES -DLTC_SMALL_CODE -DLTC_NO_ECC_TIMING_RESISTANT -DLTC_NO_RSA_BLINDING" "$1" "$2" "$3" || exit 1

# test build with no testing
bash testbuild.sh "NOTEST" "-DLTC_NO_TEST" "$1" "$2" "$3" || exit 1

# test build with no file routines
bash testbuild.sh "NOFILE" "-DLTC_NO_FILE" "$1" "$2" "$3" || exit 1

# $Source$   
# $Revision$   
# $Date$ 
