/* FriBidi
 * fribidi-benchmark.c - command line benchmark tool for libfribidi
 *
 * $Id: fribidi-benchmark.c,v 1.8 2009-04-14 03:49:52 behdad Exp $
 * $Author: behdad $
 * $Date: 2009-04-14 03:49:52 $
 * $Revision: 1.8 $
 * $Source: /home/behdad/src/fdo/fribidi/togit/git/../fribidi/fribidi2/bin/fribidi-benchmark.c,v $
 *
 * Authors:
 *   Behdad Esfahbod, 2001, 2002, 2004
 *   Dov Grobgeld, 1999, 2000
 *
 * Copyright (C) 2004 Sharif FarsiWeb, Inc
 * Copyright (C) 2001,2002 Behdad Esfahbod
 * Copyright (C) 1999,2000 Dov Grobgeld
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library, in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA
 * 
 * For licensing issues, contact <license@farsiweb.info>.
 */

#include <common.h>

#include <fribidi.h>

#include <stdio.h>
#if STDC_HEADERS+0
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#if HAVE_STRING_H+0
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#if HAVE_STRINGS_H+0
# include <strings.h>
#endif
#if HAVE_SYS_TIMES_H+0
# include <sys/times.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "getopt.h"

#define appname "fribidi_benchmark"

#define MAX_STR_LEN 1000
#define NUM_ITER 2000

static void
die2 (
  const char *fmt,
  const char *arg
)
{
  fprintf (stderr, "%s: ", appname);
  if (fmt)
    fprintf (stderr, fmt, arg);
  fprintf (stderr, "Try `%s --help' for more information.\n", appname);
  exit (-1);
}

#define TEST_STRING \
  "a THE QUICK -123,456 (FOX JUMPS ) DOG the quick !1@7#4&5^ over the dog " \
  "123,456 OVER THE 5%+ 4.0 LAZY"
#define TEST_STRING_EXPLICIT \
  "this is _LJUST_o a _lsimple _Rte%ST_o th_oat  HAS A _LPDF missing" \
  "AnD hOw_L AbOuT, 123,987 tHiS_o a GO_oOD - _L_oTE_oST. " \
  "here_L is_o_o_o _R a good one_o And _r 123,987_LT_oHE_R next_o oNE:" \
  "_R_r and the last _LONE_o IS THE _rbest _lONE and" \
  "a _L_L_L_LL_L_L_L_L_L_L_L_L_Rbug_o_o_o_o_o_o" \
  "_R_r and the last _LONE_o IS THE _rbest _lONE and" \
  "A REAL BIG_l_o BUG! _L _l_r_R_L_laslaj siw_o_Rlkj sslk" \
  "a _L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_Rbug" \
  "here_L is_o_o_o _R ab  one_o _r 123,987_LT_oHE_R t_o oNE:" \

static void
help (
  void
)
{
  printf
    ("Usage: " appname " [OPTION]...\n"
     "A program for benchmarking the speed of the " FRIBIDI_NAME
     " library.\n" "\n"
     "  -h, --help            Display this information and exit\n"
     "  -V, --version         Display version information and exit\n"
     "  -n, --niter N         Number of iterations. Default is %d.\n"
     "\nReport bugs online at\n<" FRIBIDI_BUGREPORT ">.\n", NUM_ITER);
  exit (0);
}

static void
version (
  void
)
{
  printf (appname " %s", fribidi_version_info);
  exit (0);
}

static double
utime (
  void
)
{
#ifdef _WIN32
  FILETIME creationTime, exitTime, kernelTime, userTime;
  HANDLE currentProcess = GetCurrentProcess();
  if (GetProcessTimes(currentProcess, &creationTime, &exitTime, &kernelTime, &userTime))
  {
      unsigned __int64 myTime = userTime.dwHighDateTime;
      myTime = (myTime << 32) | userTime.dwLowDateTime;
      return 1e-7 * myTime;
  }
  else
      return 0.0;
#else /* !_WIN32 */
#if HAVE_SYS_TIMES_H+0
  struct tms tb;
  times (&tb);
  return 0.01 * tb.tms_utime;
#else
#warning Please fill in here to use other functions for determining time.
  return 0.0;
#endif
#endif
}

static void
benchmark (
  const char *S_,
  int niter
)
{
  int len, i;
  FriBidiChar us[MAX_STR_LEN], out_us[MAX_STR_LEN];
  FriBidiStrIndex positionLtoV[MAX_STR_LEN], positionVtoL[MAX_STR_LEN];
  FriBidiLevel embedding_list[MAX_STR_LEN];
  FriBidiParType base;
  double time0, time1;

  {
    int j;
    len = strlen (S_);
    for (i = 0, j = 0; i < len; i++)
      {
	if (S_[i] == '_')
	  switch (S_[++i])
	    {
	    case '>':
	      us[j++] = FRIBIDI_CHAR_LRM;
	      break;
	    case '<':
	      us[j++] = FRIBIDI_CHAR_RLM;
	      break;
	    case 'l':
	      us[j++] = FRIBIDI_CHAR_LRE;
	      break;
	    case 'r':
	      us[j++] = FRIBIDI_CHAR_RLE;
	      break;
	    case 'L':
	      us[j++] = FRIBIDI_CHAR_LRO;
	      break;
	    case 'R':
	      us[j++] = FRIBIDI_CHAR_RLO;
	      break;
	    case 'o':
	      us[j++] = FRIBIDI_CHAR_PDF;
	      break;
	    case '_':
	      us[j++] = '_';
	      break;
	    default:
	      us[j++] = '_';
	      i--;
	      break;
	    }
	else
	  us[j++] = S_[i];
	if (us[j] >= 'A' && us[j] <= 'F')
	  us[j] += FRIBIDI_CHAR_ARABIC_ALEF - 'A';
	else if (us[j] >= 'G' && us[j] <= 'Z')
	  us[j] += FRIBIDI_CHAR_HEBREW_ALEF - 'G';
	else if (us[j] >= '6' && us[j] <= '9')
	  us[j] += FRIBIDI_CHAR_ARABIC_ZERO - '0';
      }
    len = j;
  }

  /* Start timer */
  time0 = utime ();

  for (i = 0; i < niter; i++)
    {
      /* Create a bidi string */
      base = FRIBIDI_PAR_ON;
      if (!fribidi_log2vis (us, len, &base,
			    /* output */
			    out_us, positionVtoL, positionLtoV,
			    embedding_list))
	die2
	  ("something failed in fribidi_log2vis.\n"
	   "perhaps memory allocation failure.", NULL);
    }

  /* stop timer */
  time1 = utime ();

  /* output result */
  printf ("Length = %d\n", len);
  printf ("Iterations = %d\n", niter);
  printf ("%d len*iterations in %f seconds\n", len * niter, time1 - time0);
  printf ("= %.0f kilo.length.iterations/second\n",
	  1.0 * len * niter / 1000 / (time1 - time0));

  return;
}

int
main (
  int argc,
  char *argv[]
)
{
  int niter = NUM_ITER;

  /* Parse the command line */
  argv[0] = appname;
  while (1)
    {
      int option_index = 0, c;
      static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"version", 0, 0, 'V'},
	{"niter", 0, 0, 'n'},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "hVn:", long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  break;
	case 'h':
	  help ();
	  break;
	case 'V':
	  version ();
	  break;
	case 'n':
	  niter = atoi (optarg);
	  if (niter <= 0)
	    die2 ("invalid number of iterations `%s'\n", optarg);
	  break;
	case ':':
	case '?':
	  die2 (NULL, NULL);
	  break;
	default:
	  break;
	}
    }

  printf ("* Without explicit marks:\n");
  benchmark (TEST_STRING, niter);
  printf ("\n");
  printf ("* With explicit marks:\n");
  benchmark (TEST_STRING_EXPLICIT, niter);

  return 0;
}

/* Editor directions:
 * vim:textwidth=78:tabstop=8:shiftwidth=2:autoindent:cindent
 */
