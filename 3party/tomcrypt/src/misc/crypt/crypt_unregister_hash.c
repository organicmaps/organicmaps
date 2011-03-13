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
  @file crypt_unregister_hash.c
  Unregister a hash, Tom St Denis
*/

/**
  Unregister a hash from the descriptor table
  @param hash   The hash descriptor to remove
  @return CRYPT_OK on success
*/
int unregister_hash(const struct ltc_hash_descriptor *hash)
{
   int x;

   LTC_ARGCHK(hash != NULL);

   /* is it already registered? */
   LTC_MUTEX_LOCK(&ltc_hash_mutex);
   for (x = 0; x < TAB_SIZE; x++) {
       if (XMEMCMP(&hash_descriptor[x], hash, sizeof(struct ltc_hash_descriptor)) == 0) {
          hash_descriptor[x].name = NULL;
          LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
          return CRYPT_OK;
       }
   }
   LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
   return CRYPT_ERROR;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_unregister_hash.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
