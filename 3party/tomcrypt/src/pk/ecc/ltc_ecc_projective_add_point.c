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
  @file ltc_ecc_projective_add_point.c
  ECC Crypto, Tom St Denis
*/  

#if defined(LTC_MECC) && (!defined(LTC_MECC_ACCEL) || defined(LTM_LTC_DESC))

/**
   Add two ECC points
   @param P        The point to add
   @param Q        The point to add
   @param R        [out] The destination of the double
   @param modulus  The modulus of the field the ECC curve is in
   @param mp       The "b" value from montgomery_setup()
   @return CRYPT_OK on success
*/
int ltc_ecc_projective_add_point(ecc_point *P, ecc_point *Q, ecc_point *R, void *modulus, void *mp)
{
   void  *t1, *t2, *x, *y, *z;
   int    err;

   LTC_ARGCHK(P       != NULL);
   LTC_ARGCHK(Q       != NULL);
   LTC_ARGCHK(R       != NULL);
   LTC_ARGCHK(modulus != NULL);
   LTC_ARGCHK(mp      != NULL);

   if ((err = mp_init_multi(&t1, &t2, &x, &y, &z, NULL)) != CRYPT_OK) {
      return err;
   }
   
   /* should we dbl instead? */
   if ((err = mp_sub(modulus, Q->y, t1)) != CRYPT_OK)                          { goto done; }

   if ( (mp_cmp(P->x, Q->x) == LTC_MP_EQ) && 
        (Q->z != NULL && mp_cmp(P->z, Q->z) == LTC_MP_EQ) &&
        (mp_cmp(P->y, Q->y) == LTC_MP_EQ || mp_cmp(P->y, t1) == LTC_MP_EQ)) {
        mp_clear_multi(t1, t2, x, y, z, NULL);
        return ltc_ecc_projective_dbl_point(P, R, modulus, mp);
   }

   if ((err = mp_copy(P->x, x)) != CRYPT_OK)                                   { goto done; }
   if ((err = mp_copy(P->y, y)) != CRYPT_OK)                                   { goto done; }
   if ((err = mp_copy(P->z, z)) != CRYPT_OK)                                   { goto done; }

   /* if Z is one then these are no-operations */
   if (Q->z != NULL) {
      /* T1 = Z' * Z' */
      if ((err = mp_sqr(Q->z, t1)) != CRYPT_OK)                                { goto done; }
      if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)           { goto done; }
      /* X = X * T1 */
      if ((err = mp_mul(t1, x, x)) != CRYPT_OK)                                { goto done; }
      if ((err = mp_montgomery_reduce(x, modulus, mp)) != CRYPT_OK)            { goto done; }
      /* T1 = Z' * T1 */
      if ((err = mp_mul(Q->z, t1, t1)) != CRYPT_OK)                            { goto done; }
      if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)           { goto done; }
      /* Y = Y * T1 */
      if ((err = mp_mul(t1, y, y)) != CRYPT_OK)                                { goto done; }
      if ((err = mp_montgomery_reduce(y, modulus, mp)) != CRYPT_OK)            { goto done; }
   }

   /* T1 = Z*Z */
   if ((err = mp_sqr(z, t1)) != CRYPT_OK)                                      { goto done; }
   if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)              { goto done; }
   /* T2 = X' * T1 */
   if ((err = mp_mul(Q->x, t1, t2)) != CRYPT_OK)                               { goto done; }
   if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK)              { goto done; }
   /* T1 = Z * T1 */
   if ((err = mp_mul(z, t1, t1)) != CRYPT_OK)                                  { goto done; }
   if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)              { goto done; }
   /* T1 = Y' * T1 */
   if ((err = mp_mul(Q->y, t1, t1)) != CRYPT_OK)                               { goto done; }
   if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)              { goto done; }

   /* Y = Y - T1 */
   if ((err = mp_sub(y, t1, y)) != CRYPT_OK)                                   { goto done; }
   if (mp_cmp_d(y, 0) == LTC_MP_LT) {
      if ((err = mp_add(y, modulus, y)) != CRYPT_OK)                           { goto done; }
   }
   /* T1 = 2T1 */
   if ((err = mp_add(t1, t1, t1)) != CRYPT_OK)                                 { goto done; }
   if (mp_cmp(t1, modulus) != LTC_MP_LT) {
      if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK)                         { goto done; }
   }
   /* T1 = Y + T1 */
   if ((err = mp_add(t1, y, t1)) != CRYPT_OK)                                  { goto done; }
   if (mp_cmp(t1, modulus) != LTC_MP_LT) {
      if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK)                         { goto done; }
   }
   /* X = X - T2 */
   if ((err = mp_sub(x, t2, x)) != CRYPT_OK)                                   { goto done; }
   if (mp_cmp_d(x, 0) == LTC_MP_LT) {
      if ((err = mp_add(x, modulus, x)) != CRYPT_OK)                           { goto done; }
   }
   /* T2 = 2T2 */
   if ((err = mp_add(t2, t2, t2)) != CRYPT_OK)                                 { goto done; }
   if (mp_cmp(t2, modulus) != LTC_MP_LT) {
      if ((err = mp_sub(t2, modulus, t2)) != CRYPT_OK)                         { goto done; }
   }
   /* T2 = X + T2 */
   if ((err = mp_add(t2, x, t2)) != CRYPT_OK)                                  { goto done; }
   if (mp_cmp(t2, modulus) != LTC_MP_LT) {
      if ((err = mp_sub(t2, modulus, t2)) != CRYPT_OK)                         { goto done; }
   }

   /* if Z' != 1 */
   if (Q->z != NULL) {
      /* Z = Z * Z' */
      if ((err = mp_mul(z, Q->z, z)) != CRYPT_OK)                              { goto done; }
      if ((err = mp_montgomery_reduce(z, modulus, mp)) != CRYPT_OK)            { goto done; }
   }

   /* Z = Z * X */
   if ((err = mp_mul(z, x, z)) != CRYPT_OK)                                    { goto done; }
   if ((err = mp_montgomery_reduce(z, modulus, mp)) != CRYPT_OK)               { goto done; }

   /* T1 = T1 * X  */
   if ((err = mp_mul(t1, x, t1)) != CRYPT_OK)                                  { goto done; }
   if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)              { goto done; }
   /* X = X * X */
   if ((err = mp_sqr(x, x)) != CRYPT_OK)                                       { goto done; }
   if ((err = mp_montgomery_reduce(x, modulus, mp)) != CRYPT_OK)               { goto done; }
   /* T2 = T2 * x */
   if ((err = mp_mul(t2, x, t2)) != CRYPT_OK)                                  { goto done; }
   if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK)              { goto done; }
   /* T1 = T1 * X  */
   if ((err = mp_mul(t1, x, t1)) != CRYPT_OK)                                  { goto done; }
   if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)              { goto done; }
 
   /* X = Y*Y */
   if ((err = mp_sqr(y, x)) != CRYPT_OK)                                       { goto done; }
   if ((err = mp_montgomery_reduce(x, modulus, mp)) != CRYPT_OK)               { goto done; }
   /* X = X - T2 */
   if ((err = mp_sub(x, t2, x)) != CRYPT_OK)                                   { goto done; }
   if (mp_cmp_d(x, 0) == LTC_MP_LT) {
      if ((err = mp_add(x, modulus, x)) != CRYPT_OK)                           { goto done; }
   }

   /* T2 = T2 - X */
   if ((err = mp_sub(t2, x, t2)) != CRYPT_OK)                                  { goto done; }
   if (mp_cmp_d(t2, 0) == LTC_MP_LT) {
      if ((err = mp_add(t2, modulus, t2)) != CRYPT_OK)                         { goto done; }
   } 
   /* T2 = T2 - X */
   if ((err = mp_sub(t2, x, t2)) != CRYPT_OK)                                  { goto done; }
   if (mp_cmp_d(t2, 0) == LTC_MP_LT) {
      if ((err = mp_add(t2, modulus, t2)) != CRYPT_OK)                         { goto done; }
   }
   /* T2 = T2 * Y */
   if ((err = mp_mul(t2, y, t2)) != CRYPT_OK)                                  { goto done; }
   if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK)              { goto done; }
   /* Y = T2 - T1 */
   if ((err = mp_sub(t2, t1, y)) != CRYPT_OK)                                  { goto done; }
   if (mp_cmp_d(y, 0) == LTC_MP_LT) {
      if ((err = mp_add(y, modulus, y)) != CRYPT_OK)                           { goto done; }
   }
   /* Y = Y/2 */
   if (mp_isodd(y)) {
      if ((err = mp_add(y, modulus, y)) != CRYPT_OK)                           { goto done; }
   }
   if ((err = mp_div_2(y, y)) != CRYPT_OK)                                     { goto done; }

   if ((err = mp_copy(x, R->x)) != CRYPT_OK)                                   { goto done; }
   if ((err = mp_copy(y, R->y)) != CRYPT_OK)                                   { goto done; }
   if ((err = mp_copy(z, R->z)) != CRYPT_OK)                                   { goto done; }

   err = CRYPT_OK;
done:
   mp_clear_multi(t1, t2, x, y, z, NULL);
   return err;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ltc_ecc_projective_add_point.c,v $ */
/* $Revision: 1.16 $ */
/* $Date: 2007/05/12 14:32:35 $ */

