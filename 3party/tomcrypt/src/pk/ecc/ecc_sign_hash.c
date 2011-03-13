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
  @file ecc_sign_hash.c
  ECC Crypto, Tom St Denis
*/  

#ifdef LTC_MECC

/**
  Sign a message digest
  @param in        The message digest to sign
  @param inlen     The length of the digest
  @param out       [out] The destination for the signature
  @param outlen    [in/out] The max size and resulting size of the signature
  @param prng      An active PRNG state
  @param wprng     The index of the PRNG you wish to use
  @param key       A private ECC key
  @return CRYPT_OK if successful
*/
int ecc_sign_hash(const unsigned char *in,  unsigned long inlen, 
                        unsigned char *out, unsigned long *outlen, 
                        prng_state *prng, int wprng, ecc_key *key)
{
   ecc_key       pubkey;
   void          *r, *s, *e, *p;
   int           err;

   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(key    != NULL);

   /* is this a private key? */
   if (key->type != PK_PRIVATE) {
      return CRYPT_PK_NOT_PRIVATE;
   }
   
   /* is the IDX valid ?  */
   if (ltc_ecc_is_valid_idx(key->idx) != 1) {
      return CRYPT_PK_INVALID_TYPE;
   }
   
   if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
      return err;
   }

   /* get the hash and load it as a bignum into 'e' */
   /* init the bignums */
   if ((err = mp_init_multi(&r, &s, &p, &e, NULL)) != CRYPT_OK) { 
      return err;
   }
   if ((err = mp_read_radix(p, (char *)key->dp->order, 16)) != CRYPT_OK)                      { goto errnokey; }
   if ((err = mp_read_unsigned_bin(e, (unsigned char *)in, (int)inlen)) != CRYPT_OK)          { goto errnokey; }

   /* make up a key and export the public copy */
   for (;;) {
      if ((err = ecc_make_key_ex(prng, wprng, &pubkey, key->dp)) != CRYPT_OK) {
         goto errnokey;
      }

      /* find r = x1 mod n */
      if ((err = mp_mod(pubkey.pubkey.x, p, r)) != CRYPT_OK)                 { goto error; }

      if (mp_iszero(r) == LTC_MP_YES) {
         ecc_free(&pubkey);
      } else { 
        /* find s = (e + xr)/k */
        if ((err = mp_invmod(pubkey.k, p, pubkey.k)) != CRYPT_OK)            { goto error; } /* k = 1/k */
        if ((err = mp_mulmod(key->k, r, p, s)) != CRYPT_OK)                  { goto error; } /* s = xr */
        if ((err = mp_add(e, s, s)) != CRYPT_OK)                             { goto error; } /* s = e +  xr */
        if ((err = mp_mod(s, p, s)) != CRYPT_OK)                             { goto error; } /* s = e +  xr */
        if ((err = mp_mulmod(s, pubkey.k, p, s)) != CRYPT_OK)                { goto error; } /* s = (e + xr)/k */
        ecc_free(&pubkey);
        if (mp_iszero(s) == LTC_MP_NO) {
           break;
        }
      }
   }

   /* store as SEQUENCE { r, s -- integer } */
   err = der_encode_sequence_multi(out, outlen,
                             LTC_ASN1_INTEGER, 1UL, r,
                             LTC_ASN1_INTEGER, 1UL, s,
                             LTC_ASN1_EOL, 0UL, NULL);
   goto errnokey;
error:
   ecc_free(&pubkey);
errnokey:
   mp_clear_multi(r, s, p, e, NULL);
   return err;   
}

#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_sign_hash.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */

