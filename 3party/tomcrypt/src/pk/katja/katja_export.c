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
  @file katja_export.c
  Export Katja LTC_PKCS-style keys, Tom St Denis
*/  

#ifdef MKAT

/**
    This will export either an KatjaPublicKey or KatjaPrivateKey
    @param out       [out] Destination of the packet
    @param outlen    [in/out] The max size and resulting size of the packet
    @param type      The type of exported key (PK_PRIVATE or PK_PUBLIC)
    @param key       The Katja key to export
    @return CRYPT_OK if successful
*/    
int katja_export(unsigned char *out, unsigned long *outlen, int type, katja_key *key)
{
   int           err;
   unsigned long zero=0;

   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(key    != NULL);

   /* type valid? */
   if (!(key->type == PK_PRIVATE) && (type == PK_PRIVATE)) {
      return CRYPT_PK_INVALID_TYPE;
   }

   if (type == PK_PRIVATE) {
      /* private key */
      /* output is 
            Version, n, d, p, q, d mod (p-1), d mod (q - 1), 1/q mod p, pq
       */
      if ((err = der_encode_sequence_multi(out, outlen, 
                          LTC_ASN1_SHORT_INTEGER, 1UL, &zero, 
                          LTC_ASN1_INTEGER, 1UL,  key->N, 
                          LTC_ASN1_INTEGER, 1UL,  key->d, 
                          LTC_ASN1_INTEGER, 1UL,  key->p, 
                          LTC_ASN1_INTEGER, 1UL,  key->q, 
                          LTC_ASN1_INTEGER, 1UL,  key->dP,
                          LTC_ASN1_INTEGER, 1UL,  key->dQ, 
                          LTC_ASN1_INTEGER, 1UL,  key->qP, 
                          LTC_ASN1_INTEGER, 1UL,  key->pq, 
                          LTC_ASN1_EOL,     0UL, NULL)) != CRYPT_OK) {
         return err;
      }
 
      /* clear zero and return */
      return CRYPT_OK;
   } else {
      /* public key */
      return der_encode_sequence_multi(out, outlen, 
                                 LTC_ASN1_INTEGER, 1UL, key->N, 
                                 LTC_ASN1_EOL,     0UL, NULL);
   }
}

#endif /* LTC_MRSA */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/katja/katja_export.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2007/05/12 14:32:35 $ */
