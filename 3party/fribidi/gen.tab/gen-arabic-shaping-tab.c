/* FriBidi
 * gen-arabic-shaping-tab.c - generate arabic-shaping.tab.i
 *
 * $Id: gen-arabic-shaping-tab.c,v 1.2 2006/01/31 03:23:12 behdad Exp $
 * $Author: behdad $
 * $Date: 2006/01/31 03:23:12 $
 * $Revision: 1.2 $
 * $Source: /cvs/fribidi/fribidi2/gen.tab/gen-arabic-shaping-tab.c,v $
 *
 * Author:
 *   Behdad Esfahbod, 2004, 2005
 *
 * Copyright (C) 2004 Sharif FarsiWeb, Inc
 * Copyright (C) 2004, 2005 Behdad Esfahbod
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA
 * 
 * For licensing issues, contact <license@farsiweb.info>.
 */

#include <common.h>

#include <fribidi-unicode.h>

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

#include "packtab.h"

#define appname "gen-arabic-shaping-tab"
#define outputname "arabic-shaping.tab.i"

static void
die (
  const char *msg
)
{
  fprintf (stderr, appname ": %s\n", msg);
  exit (1);
}

static void
die2 (
  const char *fmt,
  const char *p
)
{
  fprintf (stderr, appname ": ");
  fprintf (stderr, fmt, p);
  fprintf (stderr, "\n");
  exit (1);
}

static void
die3 (
  const char *fmt,
  const char *p,
  const char *q
)
{
  fprintf (stderr, appname ": ");
  fprintf (stderr, fmt, p, q);
  fprintf (stderr, "\n");
  exit (1);
}

static void
die3l (
  const char *fmt,
  unsigned long l,
  const char *p
)
{
  fprintf (stderr, appname ": ");
  fprintf (stderr, fmt, l, p);
  fprintf (stderr, "\n");
  exit (1);
}

static const char *arabic_shaping_tags[] = {
  "isolated",
  "final",
  "initial",
  "medial",
  NULL
};

static int
shape_is (
  const char *s,
  const char *type_list[]
)
{
  const char **p = type_list;
  for (; *p; p++)
    if (!strcmp (s, p[0]))
      return  p - type_list;
  return -1;
}

#define table_name "ArShap"
#define macro_name "FRIBIDI_GET_ARABIC_SHAPE_PRES"

#define START_CHAR 0x600
#define END_CHAR 0x700

static FriBidiChar table[FRIBIDI_UNICODE_CHARS][4];
static char buf[4000];
static char tag[sizeof (buf)], buf2[sizeof (buf)];
static FriBidiChar minshaped, maxshaped;

static void
clear_tab (
  void
)
{
  register FriBidiChar c;
  register int shape;

  for (c = 0; c < FRIBIDI_UNICODE_CHARS; c++)
    for (shape = 0; shape < 4; shape++)
      table[c][shape] = c;
}

static void
init (
  void
)
{
  minshaped = FRIBIDI_UNICODE_CHARS;
  maxshaped = 0;

  clear_tab ();
}

static void
read_unicode_data_txt (
  FILE *f
)
{
  unsigned long c, unshaped, l;

  l = 0;
  while (fgets (buf, sizeof buf, f))
    {
      int i, shape;
      const char *s = buf;

      l++;

      while (*s == ' ')
	s++;

      if (*s == '#' || *s == '\0' || *s == '\n')
	continue;

      i = sscanf (s, "%lx;%*[^;];%*[^;];%*[^;];%*[^;];<%[^;> ]> %lx %[^; ]", &c, tag, &unshaped, buf2);

      if (i != 3)
        continue;

      if (i != 3 || c >= FRIBIDI_UNICODE_CHARS || unshaped >= FRIBIDI_UNICODE_CHARS)
	die3l ("UnicodeData.txt: invalid input at line %ld: %s", l, s);

      shape = shape_is (tag, arabic_shaping_tags);
      if (shape >= 0)
        {
	  table[unshaped][shape] = c;
	  if (unshaped < minshaped)
	    minshaped = unshaped;
	  if (unshaped > maxshaped)
	    maxshaped = unshaped;
	}
    }
}

static void
read_data (
  const char *data_file_type[],
  const char *data_file_name[]
)
{
  FILE *f;

  for (; data_file_name[0] && data_file_type[0];
       data_file_name++, data_file_type++)
    {
      fprintf (stderr, "Reading `%s'\n", data_file_name[0]);
      if (!(f = fopen (data_file_name[0], "rt")))
	die2 ("error: cannot open `%s' for reading", data_file_name[0]);

      if (!strcmp (data_file_type[0], "UnicodeData.txt"))
	read_unicode_data_txt (f);
      else
	die2 ("error: unknown data-file type %s", data_file_type[0]);

      fclose (f);
    }

}

static void
gen_arabic_shaping_tab (
  int max_depth /* currently unused */,
  const char *data_file_type[]
)
{
  register FriBidiChar c;
  register int shape;
 
  if (maxshaped < minshaped)
    die ("error: no shaping pair found, something wrong with reading input");

  fprintf (stderr,
	   "Generating `" outputname "', it may take up to a few minutes\n");
  printf ("/* " outputname "\n * generated by " appname " (" FRIBIDI_NAME " "
	  FRIBIDI_VERSION ")\n" " * from the files %s, %s of Unicode version "
	  FRIBIDI_UNICODE_VERSION ". */\n\n", data_file_type[0],
	  data_file_type[1]);

  printf ("/*\n"
	   "  use %s(key,shape) to access your table\n\n"
	   "  required memory: %ld\n"
	   " */\n\n",
	   macro_name, (long)(maxshaped - minshaped + 1) * 4 * sizeof (FriBidiChar));

  printf ("\n" "/* *IND" "ENT-OFF* */\n\n");

  printf ("static const FriBidiChar %s[%d][%d] = {\n", table_name, maxshaped - minshaped + 1, 4);
  for (c = minshaped; c <= maxshaped; c++)
    {
      printf ("  {");
      for (shape = 0; shape < 4; shape++)
        printf ("0x%04lx,", (unsigned long)table[c][shape]);
      printf ("},\n");
    }
      

  printf ("};\n\n");

  printf ("/* *IND" "ENT-ON* */\n\n");

  printf ("#ifndef FRIBIDI_ACCESS_SHAPE_TABLE\n"
	  "# define FRIBIDI_ACCESS_SHAPE_TABLE(table,min,max,x,shape) \\\n"
	  "	(((x)<(min)||(x)>(max))?(x):(table)[(x)-(min)][(shape)])\n"
	  "#endif\n\n");
  printf ("#define %s(x,shape) "
	  "FRIBIDI_ACCESS_SHAPE_TABLE(%s, 0x%04lx, 0x%04lx, (x), (shape))\n\n",
	  macro_name, table_name, (unsigned long)minshaped, (unsigned long)maxshaped);

  printf ("/* End of generated " outputname " */\n");
}

int
main (
  int argc,
  const char **argv
)
{
  const char *data_file_type[] =
    { "UnicodeData.txt", NULL };

  if (argc < 3)
    die3 ("usage:\n  " appname " max-depth /path/to/%s /path/to/%s [junk...]",
	  data_file_type[0], data_file_type[1]);

  {
    int max_depth = atoi (argv[1]);
    const char *data_file_name[] = { NULL, NULL };
    data_file_name[0] = argv[2];

    if (max_depth < 2)
      die ("invalid depth");

    init ();
    read_data (data_file_type, data_file_name);
    gen_arabic_shaping_tab (max_depth, data_file_type);
  }

  return 0;
}
