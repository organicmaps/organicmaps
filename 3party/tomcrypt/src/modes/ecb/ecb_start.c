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
   @file ecb_start.c
   ECB implementation, start chain, Tom St Denis
*/


#ifdef LTC_ECB_MODE

/**
   Initialize a ECB context
   @param cipher      The index of the cipher desired
   @param key         The secret key 
   @param keylen      The length of the secret key (octets)
   @param num_rounds  Number of rounds in the cipher desired (0 for default)
   @param ecb         The ECB state to initialize
   @return CRYPT_OK if successful
*/
int ecb_start(int cipher, const unsigned char *key, int keylen, int num_rounds, symmetric_ECB *ecb)
{
   int err;
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(ecb != NULL);

   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }
   ecb->cipher = cipher;
   ecb->blocklen = cipher_descriptor[cipher].block_length;
   return cipher_descriptor[cipher].setup(key, keylen, num_rounds, &ecb->key);
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ecb/ecb_start.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */
