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
  @file crypt_cipher_is_valid.c
  Determine if cipher is valid, Tom St Denis
*/

/*
   Test if a cipher index is valid
   @param idx   The index of the cipher to search for
   @return CRYPT_OK if valid
*/
int cipher_is_valid(int idx)
{
   LTC_MUTEX_LOCK(&ltc_cipher_mutex);
   if (idx < 0 || idx >= TAB_SIZE || cipher_descriptor[idx].name == NULL) {
      LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
      return CRYPT_INVALID_CIPHER;
   }
   LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
   return CRYPT_OK;
}

/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_cipher_is_valid.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */
