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
#include "tomcrypt.h"

/**
  @file katja_import.c
  Import a LTC_PKCS-style Katja key, Tom St Denis
*/  

#ifdef MKAT

/**
  Import an KatjaPublicKey or KatjaPrivateKey [two-prime only, only support >= 1024-bit keys, defined in LTC_PKCS #1 v2.1]
  @param in      The packet to import from
  @param inlen   It's length (octets)
  @param key     [out] Destination for newly imported key
  @return CRYPT_OK if successful, upon error allocated memory is freed
*/
int katja_import(const unsigned char *in, unsigned long inlen, katja_key *key)
{
   int           err;
   void         *zero;

   LTC_ARGCHK(in  != NULL);
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(ltc_mp.name != NULL);

   /* init key */
   if ((err = mp_init_multi(&zero, &key->d, &key->N, &key->dQ, 
                            &key->dP, &key->qP, &key->p, &key->q, &key->pq, NULL)) != CRYPT_OK) {
      return err;
   }

   if ((err = der_decode_sequence_multi(in, inlen, 
                                  LTC_ASN1_INTEGER, 1UL, key->N, 
                                  LTC_ASN1_EOL,     0UL, NULL)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   if (mp_cmp_d(key->N, 0) == LTC_MP_EQ) {
      /* it's a private key */
      if ((err = der_decode_sequence_multi(in, inlen, 
                          LTC_ASN1_INTEGER, 1UL, zero, 
                          LTC_ASN1_INTEGER, 1UL, key->N, 
                          LTC_ASN1_INTEGER, 1UL, key->d, 
                          LTC_ASN1_INTEGER, 1UL, key->p, 
                          LTC_ASN1_INTEGER, 1UL, key->q, 
                          LTC_ASN1_INTEGER, 1UL, key->dP,
                          LTC_ASN1_INTEGER, 1UL, key->dQ, 
                          LTC_ASN1_INTEGER, 1UL, key->qP, 
                          LTC_ASN1_INTEGER, 1UL, key->pq, 
                          LTC_ASN1_EOL,     0UL, NULL)) != CRYPT_OK) {
         goto LBL_ERR;
      }
      key->type = PK_PRIVATE;
   } else {
      /* public we have N */
      key->type = PK_PUBLIC;
   }
   mp_clear(zero);
   return CRYPT_OK;
LBL_ERR:
   mp_clear_multi(zero,    key->d, key->N, key->dQ, key->dP,
                  key->qP, key->p, key->q, key->pq, NULL);
   return err;
}

#endif /* LTC_MRSA */


/* $Source: /cvs/libtom/libtomcrypt/src/pk/katja/katja_import.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2007/05/12 14:32:35 $ */
