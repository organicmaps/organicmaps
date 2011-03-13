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
  @file crypt_find_hash_id.c
  Find hash by ID, Tom St Denis
*/

/**
   Find a hash by ID number
   @param ID    The ID (not same as index) of the hash to find
   @return >= 0 if found, -1 if not present
*/
int find_hash_id(unsigned char ID)
{
   int x;
   LTC_MUTEX_LOCK(&ltc_hash_mutex);
   for (x = 0; x < TAB_SIZE; x++) {
      if (hash_descriptor[x].ID == ID) {
          x = (hash_descriptor[x].name == NULL) ? -1 : x;
          LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
          return x;
      }
   }
   LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
   return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_hash_id.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
