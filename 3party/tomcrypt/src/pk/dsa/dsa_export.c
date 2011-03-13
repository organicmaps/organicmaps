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
   @file dsa_export.c
   DSA implementation, export key, Tom St Denis
*/

#ifdef LTC_MDSA

/**
  Export a DSA key to a binary packet
  @param out    [out] Where to store the packet
  @param outlen [in/out] The max size and resulting size of the packet
  @param type   The type of key to export (PK_PRIVATE or PK_PUBLIC)
  @param key    The key to export
  @return CRYPT_OK if successful
*/
int dsa_export(unsigned char *out, unsigned long *outlen, int type, dsa_key *key)
{
   unsigned char flags[1];

   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(key    != NULL);

   /* can we store the static header?  */
   if (type == PK_PRIVATE && key->type != PK_PRIVATE) {
      return CRYPT_PK_TYPE_MISMATCH;
   }

   if (type != PK_PUBLIC && type != PK_PRIVATE) {
      return CRYPT_INVALID_ARG;
   }

   flags[0] = (type != PK_PUBLIC) ? 1 : 0;

   if (type == PK_PRIVATE) {
      return der_encode_sequence_multi(out, outlen,
                                 LTC_ASN1_BIT_STRING,   1UL, flags,
                                 LTC_ASN1_INTEGER,      1UL, key->g,
                                 LTC_ASN1_INTEGER,      1UL, key->p,
                                 LTC_ASN1_INTEGER,      1UL, key->q,
                                 LTC_ASN1_INTEGER,      1UL, key->y,
                                 LTC_ASN1_INTEGER,      1UL, key->x,
                                 LTC_ASN1_EOL,          0UL, NULL);
   } else {
      return der_encode_sequence_multi(out, outlen,
                                 LTC_ASN1_BIT_STRING,   1UL, flags,
                                 LTC_ASN1_INTEGER,      1UL, key->g,
                                 LTC_ASN1_INTEGER,      1UL, key->p,
                                 LTC_ASN1_INTEGER,      1UL, key->q,
                                 LTC_ASN1_INTEGER,      1UL, key->y,
                                 LTC_ASN1_EOL,          0UL, NULL);
   }
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/dsa/dsa_export.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:32:35 $ */
