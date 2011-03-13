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
   @file ctr_getiv.c
   CTR implementation, get IV, Tom St Denis
*/

#ifdef LTC_CTR_MODE

/**
   Get the current initial vector
   @param IV   [out] The destination of the initial vector
   @param len  [in/out]  The max size and resulting size of the initial vector
   @param ctr  The CTR state
   @return CRYPT_OK if successful
*/
int ctr_getiv(unsigned char *IV, unsigned long *len, symmetric_CTR *ctr)
{
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(len != NULL);
   LTC_ARGCHK(ctr != NULL);
   if ((unsigned long)ctr->blocklen > *len) {
      *len = ctr->blocklen;
      return CRYPT_BUFFER_OVERFLOW;
   }
   XMEMCPY(IV, ctr->ctr, ctr->blocklen);
   *len = ctr->blocklen;

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ctr/ctr_getiv.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
