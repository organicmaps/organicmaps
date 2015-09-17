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
  @file ltc_ecc_map.c
  ECC Crypto, Tom St Denis
*/  

#ifdef LTC_MECC

/**
  Map a projective jacbobian point back to affine space
  @param P        [in/out] The point to map
  @param modulus  The modulus of the field the ECC curve is in
  @param mp       The "b" value from montgomery_setup()
  @return CRYPT_OK on success
*/
int ltc_ecc_map(ecc_point *P, void *modulus, void *mp)
{
   void *t1, *t2;
   int   err;

   LTC_ARGCHK(P       != NULL);
   LTC_ARGCHK(modulus != NULL);
   LTC_ARGCHK(mp      != NULL);

   if ((err = mp_init_multi(&t1, &t2, NULL)) != CRYPT_OK) {
      return CRYPT_MEM;
   }

   /* first map z back to normal */
   if ((err = mp_montgomery_reduce(P->z, modulus, mp)) != CRYPT_OK)           { goto done; }

   /* get 1/z */
   if ((err = mp_invmod(P->z, modulus, t1)) != CRYPT_OK)                      { goto done; }
 
   /* get 1/z^2 and 1/z^3 */
   if ((err = mp_sqr(t1, t2)) != CRYPT_OK)                                    { goto done; }
   if ((err = mp_mod(t2, modulus, t2)) != CRYPT_OK)                           { goto done; }
   if ((err = mp_mul(t1, t2, t1)) != CRYPT_OK)                                { goto done; }
   if ((err = mp_mod(t1, modulus, t1)) != CRYPT_OK)                           { goto done; }

   /* multiply against x/y */
   if ((err = mp_mul(P->x, t2, P->x)) != CRYPT_OK)                            { goto done; }
   if ((err = mp_montgomery_reduce(P->x, modulus, mp)) != CRYPT_OK)           { goto done; }
   if ((err = mp_mul(P->y, t1, P->y)) != CRYPT_OK)                            { goto done; }
   if ((err = mp_montgomery_reduce(P->y, modulus, mp)) != CRYPT_OK)           { goto done; }
   if ((err = mp_set(P->z, 1)) != CRYPT_OK)                                   { goto done; }

   err = CRYPT_OK;
done:
   mp_clear_multi(t1, t2, NULL);
   return err;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */

