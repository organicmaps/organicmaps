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
  @file crypt_unregister_prng.c
  Unregister a PRNG, Tom St Denis
*/

/**
  Unregister a PRNG from the descriptor table
  @param prng   The PRNG descriptor to remove
  @return CRYPT_OK on success
*/
int unregister_prng(const struct ltc_prng_descriptor *prng)
{
   int x;

   LTC_ARGCHK(prng != NULL);
 
   /* is it already registered? */
   LTC_MUTEX_LOCK(&ltc_prng_mutex);
   for (x = 0; x < TAB_SIZE; x++) {
       if (XMEMCMP(&prng_descriptor[x], prng, sizeof(struct ltc_prng_descriptor)) != 0) {
          prng_descriptor[x].name = NULL;
          LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
          return CRYPT_OK;
       }
   }
   LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
   return CRYPT_ERROR;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_unregister_prng.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
