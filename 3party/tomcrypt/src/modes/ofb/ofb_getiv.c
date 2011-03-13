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
   @file ofb_getiv.c
   OFB implementation, get IV, Tom St Denis
*/

#ifdef LTC_OFB_MODE

/**
   Get the current initial vector
   @param IV   [out] The destination of the initial vector
   @param len  [in/out]  The max size and resulting size of the initial vector
   @param ofb  The OFB state
   @return CRYPT_OK if successful
*/
int ofb_getiv(unsigned char *IV, unsigned long *len, symmetric_OFB *ofb)
{
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(len != NULL);
   LTC_ARGCHK(ofb != NULL);
   if ((unsigned long)ofb->blocklen > *len) {
      *len = ofb->blocklen;
      return CRYPT_BUFFER_OVERFLOW;
   }
   XMEMCPY(IV, ofb->IV, ofb->blocklen);
   *len = ofb->blocklen;

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ofb/ofb_getiv.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
