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
   @file dsa_import.c
   DSA implementation, import a DSA key, Tom St Denis
*/

#ifdef LTC_MDSA

/**
   Import a DSA key 
   @param in       The binary packet to import from
   @param inlen    The length of the binary packet
   @param key      [out] Where to store the imported key
   @return CRYPT_OK if successful, upon error this function will free all allocated memory
*/
int dsa_import(const unsigned char *in, unsigned long inlen, dsa_key *key)
{
   unsigned char flags[1];
   int           err;

   LTC_ARGCHK(in  != NULL);
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(ltc_mp.name != NULL);

   /* init key */
   if (mp_init_multi(&key->p, &key->g, &key->q, &key->x, &key->y, NULL) != CRYPT_OK) {
      return CRYPT_MEM;
   }

   /* get key type */
   if ((err = der_decode_sequence_multi(in, inlen,
                                  LTC_ASN1_BIT_STRING, 1UL, flags,
                                  LTC_ASN1_EOL, 0UL, NULL)) != CRYPT_OK) {
      goto error;
   }

   if (flags[0] == 1) {
      if ((err = der_decode_sequence_multi(in, inlen,
                                 LTC_ASN1_BIT_STRING,   1UL, flags,
                                 LTC_ASN1_INTEGER,      1UL, key->g,
                                 LTC_ASN1_INTEGER,      1UL, key->p,
                                 LTC_ASN1_INTEGER,      1UL, key->q,
                                 LTC_ASN1_INTEGER,      1UL, key->y,
                                 LTC_ASN1_INTEGER,      1UL, key->x,
                                 LTC_ASN1_EOL,          0UL, NULL)) != CRYPT_OK) {
         goto error;
      }
      key->type = PK_PRIVATE;
   } else {
      if ((err = der_decode_sequence_multi(in, inlen,
                                 LTC_ASN1_BIT_STRING,   1UL, flags,
                                 LTC_ASN1_INTEGER,      1UL, key->g,
                                 LTC_ASN1_INTEGER,      1UL, key->p,
                                 LTC_ASN1_INTEGER,      1UL, key->q,
                                 LTC_ASN1_INTEGER,      1UL, key->y,
                                 LTC_ASN1_EOL,          0UL, NULL)) != CRYPT_OK) {
         goto error;
      }
      key->type = PK_PUBLIC;
  }
  key->qord = mp_unsigned_bin_size(key->q);

  if (key->qord >= LTC_MDSA_MAX_GROUP || key->qord <= 15 ||
      (unsigned long)key->qord >= mp_unsigned_bin_size(key->p) || (mp_unsigned_bin_size(key->p) - key->qord) >= LTC_MDSA_DELTA) {
      err = CRYPT_INVALID_PACKET;
      goto error;
   }

  return CRYPT_OK;
error: 
   mp_clear_multi(key->p, key->g, key->q, key->x, key->y, NULL);
   return err;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/dsa/dsa_import.c,v $ */
/* $Revision: 1.14 $ */
/* $Date: 2007/05/12 14:32:35 $ */
