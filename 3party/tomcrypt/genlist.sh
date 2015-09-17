#!/bin/bash
# aes_tab.o is a pseudo object as it's made from aes.o and MPI is optional
export a=`echo -n "src/ciphers/aes/aes_enc.o " ; find ./src -type f -name "*.c" -not -name "*tab.c" | sort | sed -e 'sE\./EE' | sed -e 's/\.c/\.o/' | xargs`
perl ./parsenames.pl OBJECTS "$a"
export a=`find src/headers -type f -name "*.h" | sort | xargs`
perl ./parsenames.pl HEADERS "$a"

# $Source$
# $Revision$
# $Date$
