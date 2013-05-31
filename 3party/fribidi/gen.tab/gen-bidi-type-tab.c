/* FriBidi
 * gen-bidi-type-tab.c - generate bidi-type.tab.i
 *
 * $Id: gen-bidi-type-tab.c,v 1.18 2006-01-31 03:23:12 behdad Exp $
 * $Author: behdad $
 * $Date: 2006-01-31 03:23:12 $
 * $Revision: 1.18 $
 * $Source: /home/behdad/src/fdo/fribidi/togit/git/../fribidi/fribidi2/gen.tab/gen-bidi-type-tab.c,v $
 *
 * Author:
 *   Behdad Esfahbod, 2001, 2002, 2004
 *
 * Copyright (C) 2004 Sharif FarsiWeb, Inc
 * Copyright (C) 2001,2002,2004 Behdad Esfahbod
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

#define appname "gen-bidi-type-tab"
#define outputname "bidi-type.tab.i"

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
  unsigned long l,
  const char *p
)
{
  fprintf (stderr, appname ": ");
  fprintf (stderr, fmt, l, p);
  fprintf (stderr, "\n");
  exit (1);
}

enum FriBidiCharTypeLinearEnumOffsetOne
{
# define _FRIBIDI_ADD_TYPE(TYPE,SYMBOL) TYPE,
# define _FRIBIDI_ADD_ALIAS(TYPE1,TYPE2) TYPE1 = TYPE2,
# include <fribidi-bidi-types-list.h>
# undef _FRIBIDI_ADD_ALIAS
# undef _FRIBIDI_ADD_TYPE
  NUM_TYPES
};

struct
{
  const char *name;
  int key;
}
type_names[] =
{
# define _FRIBIDI_ADD_TYPE(TYPE,SYMBOL) {STRINGIZE(TYPE), TYPE},
# define _FRIBIDI_ADD_ALIAS(TYPE1,TYPE2) _FRIBIDI_ADD_TYPE(TYPE1, SYMBOL)
# include <fribidi-bidi-types-list.h>
# undef _FRIBIDI_ADD_ALIAS
# undef _FRIBIDI_ADD_TYPE
};

#define type_names_count (sizeof (type_names) / sizeof (type_names[0]))

static const char *names[type_names_count];

static char
get_type (
  const char *s
)
{
  unsigned int i;

  for (i = 0; i < type_names_count; i++)
    if (!strcmp (s, type_names[i].name))
      return type_names[i].key;
  die2 ("bidi type name `%s' not found", s);
  return 0;
}

#define table_name "Bid"
#define macro_name "FRIBIDI_GET_BIDI_TYPE"

static signed int table[FRIBIDI_UNICODE_CHARS];
static char buf[4000];
static char tp[sizeof (buf)];

static void
init (
  void
)
{
  register int i;

  for (i = 0; i < type_names_count; i++)
    names[i] = 0;
  for (i = type_names_count - 1; i >= 0; i--)
    names[type_names[i].key] = type_names[i].name;
}

static void
clear_tab (
  void
)
{
  register FriBidiChar c;

  for (c = 0; c < FRIBIDI_UNICODE_CHARS; c++)
    table[c] = LTR;
}

static void
init_tab_unicode_data_txt (
  void
)
{
  register FriBidiChar c;

  clear_tab ();

  for (c = 0x0590; c < 0x0600; c++)
    table[c] = RTL;
  for (c = 0x07C0; c < 0x0900; c++)
    table[c] = RTL;
  for (c = 0xFB1D; c < 0xFB50; c++)
    table[c] = RTL;

  for (c = 0x0600; c < 0x07C0; c++)
    table[c] = AL;
  for (c = 0xFB50; c < 0xFDD0; c++)
    table[c] = AL;
  for (c = 0xFDF0; c < 0xFE00; c++)
    table[c] = AL;
  for (c = 0xFE70; c < 0xFF00; c++)
    table[c] = AL;

  for (c = 0x2060; c < 0x2070; c++)
    table[c] = BN;
  for (c = 0xFDD0; c < 0xFDF0; c++)
    table[c] = BN;
  for (c = 0xFFF0; c < 0xFFF9; c++)
    table[c] = BN;
  for (c = 0xFFFF; c < FRIBIDI_UNICODE_CHARS; c += 0x10000)
    table[c - 1] = table[c] = BN;

  if (FRIBIDI_UNICODE_CHARS > 0x10000)
    {
      for (c = 0x10800; c < 0x11000; c++)
	table[c] = RTL;

      for (c = 0xE0000; c < 0xE1000; c++)
	table[c] = BN;
    }
}

static void
init_tab_derived_bidi_class_txt (
  void
)
{
  clear_tab ();
}

static void
read_unicode_data_txt (
  FILE *f
)
{
  unsigned long c, l;

  init_tab_unicode_data_txt ();

  l = 0;
  while (fgets (buf, sizeof buf, f))
    {
      int i;
      const char *s = buf;

      l++;

      while (*s == ' ')
	s++;

      if (s[0] == '#' || s[0] == '\0' || s[0] == '\n')
	continue;

      i = sscanf (s, "%lx;%*[^;];%*[^;];%*[^;];%[^; ]", &c, tp);
      if (i != 2 || c >= FRIBIDI_UNICODE_CHARS)
	die3 ("invalid input at line %ld: %s", l, s);

      /* FIXME: We don't handle First..Last parts of UnicodeData.txt,
       * but it works, since all those are LTR. */
      table[c] = get_type (tp);
    }
}

static void
read_derived_bidi_class_txt (
  FILE *f
)
{
  unsigned long c, c2, l;

  init_tab_derived_bidi_class_txt ();

  l = 0;
  while (fgets (buf, sizeof buf, f))
    {
      int i;
      register char typ;
      const char *s = buf;

      l++;

      while (*s == ' ')
	s++;

      if (s[0] == '#' || s[0] == '\0' || s[0] == '\n')
	continue;

      i = sscanf (s, "%lx ; %[^; ]", &c, tp);
      if (i == 2)
	c2 = c;
      else
	i = sscanf (s, "%lx..%lx ; %[^; ]", &c, &c2, tp) - 1;

      if (i != 2 || c > c2 || c2 >= FRIBIDI_UNICODE_CHARS)
	die3 ("invalid input at line %ld: %s", l, s);

      typ = get_type (tp);
      for (; c <= c2; c++)
	table[c] = typ;
    }
}

static void
read_data (
  const char *data_file_type,
  const char *data_file_name
)
{
  FILE *f;

  fprintf (stderr, "Reading `%s'\n", data_file_name);
  if (!(f = fopen (data_file_name, "rt")))
    die2 ("error: cannot open `%s' for reading", data_file_name);

  if (!strcmp (data_file_type, "UnicodeData.txt"))
    read_unicode_data_txt (f);
  else if (!strcmp (data_file_type, "DerivedBidiClass.txt"))
    read_derived_bidi_class_txt (f);
  else
    die2 ("error: unknown data-file type %s", data_file_type);

  fclose (f);
}

static void
gen_bidi_type_tab (
  int max_depth,
  const char *data_file_type
)
{
  fprintf (stderr,
	   "Generating `" outputname "', it may take up to a few minutes\n");
  printf ("/* " outputname "\n * generated by " appname " (" FRIBIDI_NAME " "
	  FRIBIDI_VERSION ")\n" " * from the file %s of Unicode version "
	  FRIBIDI_UNICODE_VERSION ". */\n\n", data_file_type);

  printf ("#define PACKTAB_UINT8 fribidi_uint8\n"
	  "#define PACKTAB_UINT16 fribidi_uint16\n"
	  "#define PACKTAB_UINT32 fribidi_uint32\n\n");

  if (!pack_table
      (table, FRIBIDI_UNICODE_CHARS, 1, LTR, max_depth, 3, names,
       "unsigned char", table_name, macro_name, stdout))
    die ("error: insufficient memory, decrease max_depth");

  printf ("#undef PACKTAB_UINT8\n"
	  "#undef PACKTAB_UINT16\n" "#undef PACKTAB_UINT32\n\n");

  printf ("/* End of generated " outputname " */\n");
}

int
main (
  int argc,
  const char **argv
)
{
  const char *data_file_type = "UnicodeData.txt";

  if (argc < 3)
    die2 ("usage:\n  " appname " max-depth /path/to/%s [junk...]",
	  data_file_type);

  {
    int max_depth = atoi (argv[1]);
    const char *data_file_name = argv[2];

    if (max_depth < 2)
      die ("invalid depth");

    init ();
    read_data (data_file_type, data_file_name);
    gen_bidi_type_tab (max_depth, data_file_type);
  }

  return 0;
}
