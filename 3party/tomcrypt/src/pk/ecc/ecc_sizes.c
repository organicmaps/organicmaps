/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */
#include "tomcrypt.h"

/**
  @file ecc_sizes.c
  ECC Crypto, Tom St Denis
*/  

#ifdef LTC_MECC

void ecc_sizes(int *low, int *high)
{
 int i;
 LTC_ARGCHKVD(low  != NULL);
 LTC_ARGCHKVD(high != NULL);

 *low = INT_MAX;
 *high = 0;
 for (i = 0; ltc_ecc_sets[i].size != 0; i++) {
     if (ltc_ecc_sets[i].size < *low)  {
        *low  = ltc_ecc_sets[i].size;
     }
     if (ltc_ecc_sets[i].size > *high) {
        *high = ltc_ecc_sets[i].size;
     }
 }
}

#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_sizes.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */

