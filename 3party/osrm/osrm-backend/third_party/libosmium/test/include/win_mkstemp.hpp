/*
 * mkstemp.c
 *
 * Provides a trivial replacement for the POSIX `mkstemp()' function,
 * suitable for use in MinGW (Win32) applications.
 *
 * This file is part of the MinGW32 package set.
 *
 * Contributed by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Patched to VS2013 by alex85k
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef WIN_MKSTEMP_H
#define WIN_MKSTEMP_H

#include <stdio.h>
#include <fcntl.h>
#include <share.h>

inline int mkstemp( char *templ )
{
  int maxtry = 26, rtn = -1;

  while( maxtry-- && (rtn < 0) )
  {
    char *r = _mktemp( templ );
    if( r == NULL )
      return -1;
    rtn = sopen( r, O_RDWR | O_CREAT | O_EXCL | O_BINARY, SH_DENYRW, 0600 );
  }
  return rtn;
}
#endif
