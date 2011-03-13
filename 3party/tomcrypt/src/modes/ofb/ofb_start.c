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
   @file ofb_start.c
   OFB implementation, start chain, Tom St Denis
*/


#ifdef LTC_OFB_MODE

/**
   Initialize a OFB context
   @param cipher      The index of the cipher desired
   @param IV          The initial vector
   @param key         The secret key 
   @param keylen      The length of the secret key (octets)
   @param num_rounds  Number of rounds in the cipher desired (0 for default)
   @param ofb         The OFB state to initialize
   @return CRYPT_OK if successful
*/
int ofb_start(int cipher, const unsigned char *IV, const unsigned char *key, 
              int keylen, int num_rounds, symmetric_OFB *ofb)
{
   int x, err;

   LTC_ARGCHK(IV != NULL);
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(ofb != NULL);

   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

   /* copy details */
   ofb->cipher = cipher;
   ofb->blocklen = cipher_descriptor[cipher].block_length;
   for (x = 0; x < ofb->blocklen; x++) {
       ofb->IV[x] = IV[x];
   }

   /* init the cipher */
   ofb->padlen = ofb->blocklen;
   return cipher_descriptor[cipher].setup(key, keylen, num_rounds, &ofb->key);
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ofb/ofb_start.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */
