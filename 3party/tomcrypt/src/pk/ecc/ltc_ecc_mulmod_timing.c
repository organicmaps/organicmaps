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
  @file ltc_ecc_mulmod_timing.c
  ECC Crypto, Tom St Denis
*/  

#ifdef LTC_MECC

#ifdef LTC_ECC_TIMING_RESISTANT

/**
   Perform a point multiplication  (timing resistant)
   @param k    The scalar to multiply by
   @param G    The base point
   @param R    [out] Destination for kG
   @param modulus  The modulus of the field the ECC curve is in
   @param map      Boolean whether to map back to affine or not (1==map, 0 == leave in projective)
   @return CRYPT_OK on success
*/
int ltc_ecc_mulmod(void *k, ecc_point *G, ecc_point *R, void *modulus, int map)
{
   ecc_point *tG, *M[3];
   int        i, j, err;
   void       *mu, *mp;
   unsigned long buf;
   int        first, bitbuf, bitcpy, bitcnt, mode, digidx;

   LTC_ARGCHK(k       != NULL);
   LTC_ARGCHK(G       != NULL);
   LTC_ARGCHK(R       != NULL);
   LTC_ARGCHK(modulus != NULL);

   /* init montgomery reduction */
   if ((err = mp_montgomery_setup(modulus, &mp)) != CRYPT_OK) {
      return err;
   }
   if ((err = mp_init(&mu)) != CRYPT_OK) {
      mp_montgomery_free(mp);
      return err;
   }
   if ((err = mp_montgomery_normalization(mu, modulus)) != CRYPT_OK) {
      mp_clear(mu);
      mp_montgomery_free(mp);
      return err;
   }

  /* alloc ram for window temps */
  for (i = 0; i < 3; i++) {
      M[i] = ltc_ecc_new_point();
      if (M[i] == NULL) {
         for (j = 0; j < i; j++) {
             ltc_ecc_del_point(M[j]);
         }
         mp_clear(mu);
         mp_montgomery_free(mp);
         return CRYPT_MEM;
      }
  }

   /* make a copy of G incase R==G */
   tG = ltc_ecc_new_point();
   if (tG == NULL)                                                                   { err = CRYPT_MEM; goto done; }

   /* tG = G  and convert to montgomery */
   if ((err = mp_mulmod(G->x, mu, modulus, tG->x)) != CRYPT_OK)                      { goto done; }
   if ((err = mp_mulmod(G->y, mu, modulus, tG->y)) != CRYPT_OK)                      { goto done; }
   if ((err = mp_mulmod(G->z, mu, modulus, tG->z)) != CRYPT_OK)                      { goto done; }
   mp_clear(mu);
   mu = NULL;
   
   /* calc the M tab */
   /* M[0] == G */
   if ((err = mp_copy(tG->x, M[0]->x)) != CRYPT_OK)                                  { goto done; }
   if ((err = mp_copy(tG->y, M[0]->y)) != CRYPT_OK)                                  { goto done; }
   if ((err = mp_copy(tG->z, M[0]->z)) != CRYPT_OK)                                  { goto done; }
   /* M[1] == 2G */
   if ((err = ltc_mp.ecc_ptdbl(tG, M[1], modulus, mp)) != CRYPT_OK)                  { goto done; }

   /* setup sliding window */
   mode   = 0;
   bitcnt = 1;
   buf    = 0;
   digidx = mp_get_digit_count(k) - 1;
   bitcpy = bitbuf = 0;
   first  = 1;

   /* perform ops */
   for (;;) {
     /* grab next digit as required */
      if (--bitcnt == 0) {
         if (digidx == -1) {
            break;
         }
         buf    = mp_get_digit(k, digidx);
         bitcnt = (int) MP_DIGIT_BIT;
         --digidx;
      }

      /* grab the next msb from the ltiplicand */
      i = (buf >> (MP_DIGIT_BIT - 1)) & 1;
      buf <<= 1;

      if (mode == 0 && i == 0) {
         /* dummy operations */
         if ((err = ltc_mp.ecc_ptadd(M[0], M[1], M[2], modulus, mp)) != CRYPT_OK)    { goto done; }
         if ((err = ltc_mp.ecc_ptdbl(M[1], M[2], modulus, mp)) != CRYPT_OK)          { goto done; }
         continue;
      }

      if (mode == 0 && i == 1) {
         mode = 1;
         /* dummy operations */
         if ((err = ltc_mp.ecc_ptadd(M[0], M[1], M[2], modulus, mp)) != CRYPT_OK)    { goto done; }
         if ((err = ltc_mp.ecc_ptdbl(M[1], M[2], modulus, mp)) != CRYPT_OK)          { goto done; }
         continue;
      }

      if ((err = ltc_mp.ecc_ptadd(M[0], M[1], M[i^1], modulus, mp)) != CRYPT_OK)     { goto done; }
      if ((err = ltc_mp.ecc_ptdbl(M[i], M[i], modulus, mp)) != CRYPT_OK)             { goto done; }
   }

   /* copy result out */
   if ((err = mp_copy(M[0]->x, R->x)) != CRYPT_OK)                                   { goto done; }
   if ((err = mp_copy(M[0]->y, R->y)) != CRYPT_OK)                                   { goto done; }
   if ((err = mp_copy(M[0]->z, R->z)) != CRYPT_OK)                                   { goto done; }

   /* map R back from projective space */
   if (map) {
      err = ltc_ecc_map(R, modulus, mp);
   } else {
      err = CRYPT_OK;
   }
done:
   if (mu != NULL) {
      mp_clear(mu);
   }
   mp_montgomery_free(mp);
   ltc_ecc_del_point(tG);
   for (i = 0; i < 3; i++) {
       ltc_ecc_del_point(M[i]);
   }
   return err;
}

#endif
#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_mulmod_timing.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2007/05/12 14:32:35 $ */

