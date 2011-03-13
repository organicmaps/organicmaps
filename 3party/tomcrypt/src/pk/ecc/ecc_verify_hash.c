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
  @file ecc_verify_hash.c
  ECC Crypto, Tom St Denis
*/  

#ifdef LTC_MECC

/* verify 
 *
 * w  = s^-1 mod n
 * u1 = xw 
 * u2 = rw
 * X = u1*G + u2*Q
 * v = X_x1 mod n
 * accept if v == r
 */

/**
   Verify an ECC signature
   @param sig         The signature to verify
   @param siglen      The length of the signature (octets)
   @param hash        The hash (message digest) that was signed
   @param hashlen     The length of the hash (octets)
   @param stat        Result of signature, 1==valid, 0==invalid
   @param key         The corresponding public ECC key
   @return CRYPT_OK if successful (even if the signature is not valid)
*/
int ecc_verify_hash(const unsigned char *sig,  unsigned long siglen,
                    const unsigned char *hash, unsigned long hashlen, 
                    int *stat, ecc_key *key)
{
   ecc_point    *mG, *mQ;
   void          *r, *s, *v, *w, *u1, *u2, *e, *p, *m;
   void          *mp;
   int           err;

   LTC_ARGCHK(sig  != NULL);
   LTC_ARGCHK(hash != NULL);
   LTC_ARGCHK(stat != NULL);
   LTC_ARGCHK(key  != NULL);

   /* default to invalid signature */
   *stat = 0;
   mp    = NULL;

   /* is the IDX valid ?  */
   if (ltc_ecc_is_valid_idx(key->idx) != 1) {
      return CRYPT_PK_INVALID_TYPE;
   }

   /* allocate ints */
   if ((err = mp_init_multi(&r, &s, &v, &w, &u1, &u2, &p, &e, &m, NULL)) != CRYPT_OK) {
      return CRYPT_MEM;
   }

   /* allocate points */
   mG = ltc_ecc_new_point();
   mQ = ltc_ecc_new_point();
   if (mQ  == NULL || mG == NULL) {
      err = CRYPT_MEM;
      goto error;
   }

   /* parse header */
   if ((err = der_decode_sequence_multi(sig, siglen,
                                  LTC_ASN1_INTEGER, 1UL, r,
                                  LTC_ASN1_INTEGER, 1UL, s,
                                  LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
      goto error;
   }

   /* get the order */
   if ((err = mp_read_radix(p, (char *)key->dp->order, 16)) != CRYPT_OK)                                { goto error; }

   /* get the modulus */
   if ((err = mp_read_radix(m, (char *)key->dp->prime, 16)) != CRYPT_OK)                                { goto error; }

   /* check for zero */
   if (mp_iszero(r) || mp_iszero(s) || mp_cmp(r, p) != LTC_MP_LT || mp_cmp(s, p) != LTC_MP_LT) {
      err = CRYPT_INVALID_PACKET;
      goto error;
   }

   /* read hash */
   if ((err = mp_read_unsigned_bin(e, (unsigned char *)hash, (int)hashlen)) != CRYPT_OK)                { goto error; }

   /*  w  = s^-1 mod n */
   if ((err = mp_invmod(s, p, w)) != CRYPT_OK)                                                          { goto error; }

   /* u1 = ew */
   if ((err = mp_mulmod(e, w, p, u1)) != CRYPT_OK)                                                      { goto error; }

   /* u2 = rw */
   if ((err = mp_mulmod(r, w, p, u2)) != CRYPT_OK)                                                      { goto error; }

   /* find mG and mQ */
   if ((err = mp_read_radix(mG->x, (char *)key->dp->Gx, 16)) != CRYPT_OK)                               { goto error; }
   if ((err = mp_read_radix(mG->y, (char *)key->dp->Gy, 16)) != CRYPT_OK)                               { goto error; }
   if ((err = mp_set(mG->z, 1)) != CRYPT_OK)                                                            { goto error; }

   if ((err = mp_copy(key->pubkey.x, mQ->x)) != CRYPT_OK)                                               { goto error; }
   if ((err = mp_copy(key->pubkey.y, mQ->y)) != CRYPT_OK)                                               { goto error; }
   if ((err = mp_copy(key->pubkey.z, mQ->z)) != CRYPT_OK)                                               { goto error; }

   /* compute u1*mG + u2*mQ = mG */
   if (ltc_mp.ecc_mul2add == NULL) {
      if ((err = ltc_mp.ecc_ptmul(u1, mG, mG, m, 0)) != CRYPT_OK)                                       { goto error; }
      if ((err = ltc_mp.ecc_ptmul(u2, mQ, mQ, m, 0)) != CRYPT_OK)                                       { goto error; }
  
      /* find the montgomery mp */
      if ((err = mp_montgomery_setup(m, &mp)) != CRYPT_OK)                                              { goto error; }

      /* add them */
      if ((err = ltc_mp.ecc_ptadd(mQ, mG, mG, m, mp)) != CRYPT_OK)                                      { goto error; }
   
      /* reduce */
      if ((err = ltc_mp.ecc_map(mG, m, mp)) != CRYPT_OK)                                                { goto error; }
   } else {
      /* use Shamir's trick to compute u1*mG + u2*mQ using half of the doubles */
      if ((err = ltc_mp.ecc_mul2add(mG, u1, mQ, u2, mG, m)) != CRYPT_OK)                                { goto error; }
   }

   /* v = X_x1 mod n */
   if ((err = mp_mod(mG->x, p, v)) != CRYPT_OK)                                                         { goto error; }

   /* does v == r */
   if (mp_cmp(v, r) == LTC_MP_EQ) {
      *stat = 1;
   }

   /* clear up and return */
   err = CRYPT_OK;
error:
   ltc_ecc_del_point(mG);
   ltc_ecc_del_point(mQ);
   mp_clear_multi(r, s, v, w, u1, u2, p, e, m, NULL);
   if (mp != NULL) { 
      mp_montgomery_free(mp);
   }
   return err;
}

#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_verify_hash.c,v $ */
/* $Revision: 1.14 $ */
/* $Date: 2007/05/12 14:32:35 $ */

