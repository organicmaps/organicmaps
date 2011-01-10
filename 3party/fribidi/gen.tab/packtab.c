/* PackTab - Pack a static table
 * Copyright (C) 2001 Behdad Esfahbod. 
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
 * For licensing issues, contact <fwpg@sharif.edu>. 
 */

/*
  8 <= N <= 2^21
  int key
  1 <= max_depth <= 21
*/

#if HAVE_CONFIG_H+0
# include <config.h>
#endif /* HAVE_CONFIG_H */

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

typedef signed int uni_table[1024 * 1024 * 2];
static int n, a, max_depth, digits, tab_width, per_row;
static long N;
signed int def_key;
static uni_table temp, x, perm, *tab;
static long pow[22], cluster, cmpcluster;
static const char *const *name, *key_type_name, *table_name, *macro_name;
static FILE *f;

static long
most_binary (
  long min,
  long max
)
{
  /* min should be less than max */
  register int i, ii;

  if (min == max)
    return max;

  for (i = 21; max < pow[i]; i--)
    ;
  ii = i;
  while (i && !((min ^ max) & pow[i]))
    i--;

  if (ii == i)
    {
      /* min is less than half of max */
      for (i = 21 - 1; min < pow[i]; i--)
	;
      i++;
      return pow[i];
    }

  return max & (pow[i] - 1);
}

static void
init (
  const signed int *table
)
{
  register int i;

  /* initialize powers of two */
  pow[0] = 1;
  for (i = 1; i <= 21; i++)
    pow[i] = pow[i - 1] << 1;

  /* reduce number of elements to get a more binary number */
  {
    long essen;

    /* find number of essential items */
    essen = N - 1;
    while (essen && table[essen] == def_key)
      essen--;
    essen++;

    N = most_binary (essen, N);
  }

  for (n = 21; N % pow[n]; n--)
    ;
  digits = (n + 3) / 4;
  for (i = 6; i; i--)
    if (pow[i] * (tab_width + 1) < 80)
      break;
  per_row = pow[i];
}

static int
compare (
  const void *r,
  const void *s
)
{
  int i;
  for (i = 0; i < cmpcluster; i++)
    if (((int *) r)[i] != ((int *) s)[i])
      return ((int *) r)[i] - ((int *) s)[i];
  return 0;
}

static int lev, best_lev, p[22], best_p[22], nn;
static long c[22], best_c[22], s, best_s;
static long t[22], best_t[22], clusters[22], best_cluster[22];

static void
found (
  void
)
{
  int i;

  if (s < best_s)
    {
      best_s = s;
      best_lev = lev;
      for (i = 0; i <= lev; i++)
	{
	  best_p[i] = p[i];
	  best_c[i] = c[i];
	  best_t[i] = t[i];
	  best_cluster[i] = clusters[i];
	}
    }
}

static void
bt (
  int node_size
)
{
  long i, j, k, y, sbak;
  long key_bytes;

  if (t[lev] == 1)
    {
      found ();
      return;
    }
  if (lev == max_depth)
    return;

  for (i = 1 - t[lev] % 2; i <= nn + (t[lev] >> nn) % 2; i++)
    {
      nn -= (p[lev] = i);
      clusters[lev] = cluster = (i && nn >= 0) ? pow[i] : t[lev];
      cmpcluster = cluster + 1;

      t[lev + 1] = (t[lev] - 1) / cluster + 1;
      for (j = 0; j < t[lev + 1]; j++)
	{
	  memmove (temp + j * cmpcluster, tab[lev] + j * cluster,
		   cluster * sizeof (tab[lev][0]));
	  temp[j * cmpcluster + cluster] = j;
	}
      qsort (temp, t[lev + 1], cmpcluster * sizeof (temp[0]), compare);
      for (j = 0; j < t[lev + 1]; j++)
	{
	  perm[j] = temp[j * cmpcluster + cluster];
	  temp[j * cmpcluster + cluster] = 0;
	}
      k = 1;
      y = 0;
      tab[lev + 1][perm[0]] = perm[0];
      for (j = 1; j < t[lev + 1]; j++)
	{
	  if (compare (temp + y, temp + y + cmpcluster))
	    {
	      k++;
	      tab[lev + 1][perm[j]] = perm[j];
	    }
	  else
	    tab[lev + 1][perm[j]] = tab[lev + 1][perm[j - 1]];
	  y += cmpcluster;
	}
      sbak = s;
      s += k * node_size * cluster;
      c[lev] = k;

      if (s >= best_s)
	{
	  s = sbak;
	  nn += i;
	  return;
	}

      key_bytes = k * cluster;
      key_bytes = key_bytes < 0xff ? 1 : key_bytes < 0xffff ? 2 : 4;
      lev++;
      bt (key_bytes);
      lev--;

      s = sbak;
      nn += i;
    }
}

static void
solve (
  void
)
{
  best_lev = max_depth + 2;
  best_s = N * a * 2;
  lev = 0;
  s = 0;
  nn = n;
  t[0] = N;
  bt (a);
}

static void
write_array (
  long max_key
)
{
  int i, j, k, y, ii, ofs;
  const char *key_type;

  if (best_t[lev] == 1)
    return;

  nn -= (i = best_p[lev]);
  cluster = best_cluster[lev];
  cmpcluster = cluster + 1;

  t[lev + 1] = best_t[lev + 1];
  for (j = 0; j < t[lev + 1]; j++)
    {
      memmove (temp + j * cmpcluster, tab[lev] + j * cluster,
	       cluster * sizeof (tab[lev][0]));
      temp[j * cmpcluster + cluster] = j;
    }
  qsort (temp, t[lev + 1], cmpcluster * sizeof (temp[0]), compare);
  for (j = 0; j < t[lev + 1]; j++)
    {
      perm[j] = temp[j * cmpcluster + cluster];
      temp[j * cmpcluster + cluster] = 0;
    }
  k = 1;
  y = 0;
  tab[lev + 1][perm[0]] = x[0] = perm[0];
  for (j = 1; j < t[lev + 1]; j++)
    {
      if (compare (temp + y, temp + y + cmpcluster))
	{
	  x[k] = perm[j];
	  tab[lev + 1][perm[j]] = x[k];
	  k++;
	}
      else
	tab[lev + 1][perm[j]] = tab[lev + 1][perm[j - 1]];
      y += cmpcluster;
    }

  i = 0;
  for (ii = 1; ii < k; ii++)
    if (x[ii] < x[i])
      i = ii;

  key_type = !lev ? key_type_name :
    max_key <= 0xff ? "PACKTAB_UINT8" :
    max_key <= 0xffff ? "PACKTAB_UINT16" : "PACKTAB_UINT32";
  fprintf (f, "static const %s %sLev%d[%ld*%d] = {", key_type, table_name,
	   best_lev - lev - 1, cluster, k);
  ofs = 0;
  for (ii = 0; ii < k; ii++)
    {
      int kk, jj;
      fprintf (f, "\n#define %sLev%d_%0*lX 0x%0X", table_name,
	       best_lev - lev - 1, digits, x[i] * pow[n - nn], ofs);
      kk = x[i] * cluster;
      if (!lev)
	if (name)
	  for (j = 0; j < cluster; j++)
	    {
	      if (!(j % per_row) && j != cluster - 1)
		fprintf (f, "\n  ");
	      fprintf (f, "%*s,", tab_width, name[tab[lev][kk++]]);
	    }
	else
	  for (j = 0; j < cluster; j++)
	    {
	      if (!(j % per_row) && j != cluster - 1)
		fprintf (f, "\n  ");
	      fprintf (f, "%*d,", tab_width, tab[lev][kk++]);
	    }
      else
	for (j = 0; j < cluster; j++, kk++)
	  fprintf (f, "\n  %sLev%d_%0*lX,  /* %0*lX..%0*lX */", table_name,
		   best_lev - lev, digits,
		   tab[lev][kk] * pow[n - nn - best_p[lev]], digits,
		   x[i] * pow[n - nn] + j * pow[n - nn - best_p[lev]], digits,
		   x[i] * pow[n - nn] + (j + 1) * pow[n - nn - best_p[lev]] -
		   1);
      ofs += cluster;
      jj = i;
      for (j = 0; j < k; j++)
	if (x[j] > x[i] && (x[j] < x[jj] || jj == i))
	  jj = j;
      i = jj;
    }
  fprintf (f, "\n};\n\n");
  lev++;
  write_array (cluster * k);
  lev--;
}

static void
write_source (
  void
)
{
  int i, j;

  lev = 0;
  s = 0;
  nn = n;
  t[0] = N;
  fprintf (f, "\n" "/* *IND" "ENT-OFF* */\n\n");
  write_array (0);
  fprintf (f, "/* *IND" "ENT-ON* */\n\n");

  fprintf (f, "#define %s(x) \\\n", macro_name);
  fprintf (f, "\t((x) >= 0x%lx ? ", N);
  if (name)
    fprintf (f, "%s", name[def_key]);
  else
    fprintf (f, "%d", def_key);
  fprintf (f, " : ");
  j = 0;
  for (i = best_lev - 1; i >= 0; i--)
    {
      fprintf (f, " \\\n\t%sLev%d[((x)", table_name, i);
      if (j != 0)
	fprintf (f, " >> %d", j);
      if (i)
	fprintf (f, " & 0x%02lx) +", pow[best_p[best_lev - 1 - i]] - 1);
      j += best_p[best_lev - 1 - i];
    }
  fprintf (f, ")");
  for (i = 0; i < best_lev; i++)
    fprintf (f, "]");
  fprintf (f, ")\n\n");
}

static void
write_out (
  void
)
{
  int i;
  fprintf (f, "/*\n"
	   "  generated by packtab.c version %d\n\n"
	   "  use %s(key) to access your table\n\n"
	   "  assumed sizeof(%s): %d\n"
	   "  required memory: %ld\n"
	   "  lookups: %d\n"
	   "  partition shape: %s",
	   packtab_version, macro_name, key_type_name, a, best_s, best_lev,
	   table_name);
  for (i = best_lev - 1; i >= 0; i--)
    fprintf (f, "[%ld]", best_cluster[i]);
  fprintf (f, "\n" "  different table entries:");
  for (i = best_lev - 1; i >= 0; i--)
    fprintf (f, " %ld", best_c[i]);
  fprintf (f, "\n*/\n");
  write_source ();
}

int
pack_table (
  const signed int *base,
  long key_num,
  int key_size,
  signed int default_key,
  int p_max_depth,
  int p_tab_width,
  const char *const *p_name,
  const char *p_key_type_name,
  const char *p_table_name,
  const char *p_macro_name,
  FILE *out
)
{
  N = key_num;
  a = key_size;
  def_key = default_key;
  max_depth = p_max_depth;
  tab_width = p_tab_width;
  name = p_name;
  key_type_name = p_key_type_name;
  table_name = p_table_name;
  macro_name = p_macro_name;
  f = out;
  init (base);
  if (!(tab = malloc ((n + 1) * sizeof (tab[0]))))
    return 0;
  memmove (tab[0], base, N * sizeof (base[0]));
  solve ();
  write_out ();
  free (tab);
  return 1;
}

/* End of packtab.c */
