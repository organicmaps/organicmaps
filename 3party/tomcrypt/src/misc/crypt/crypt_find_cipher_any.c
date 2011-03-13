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
  @file crypt_find_cipher_any.c
  Find a cipher in the descriptor tables, Tom St Denis
*/

/**
   Find a cipher flexibly.  First by name then if not present by block and key size 
   @param name        The name of the cipher desired
   @param blocklen    The minimum length of the block cipher desired (octets)
   @param keylen      The minimum length of the key size desired (octets)
   @return >= 0 if found, -1 if not present
*/
int find_cipher_any(const char *name, int blocklen, int keylen)
{
   int x;

   LTC_ARGCHK(name != NULL);

   x = find_cipher(name);
   if (x != -1) return x;

   LTC_MUTEX_LOCK(&ltc_cipher_mutex);
   for (x = 0; x < TAB_SIZE; x++) {
       if (cipher_descriptor[x].name == NULL) {
          continue;
       }
       if (blocklen <= (int)cipher_descriptor[x].block_length && keylen <= (int)cipher_descriptor[x].max_key_length) {
          LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
          return x;
       }
   }
   LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
   return -1;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_find_cipher_any.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */
