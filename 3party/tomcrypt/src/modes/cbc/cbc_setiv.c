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
   @file cbc_setiv.c
   CBC implementation, set IV, Tom St Denis
*/


#ifdef LTC_CBC_MODE

/**
   Set an initial vector
   @param IV   The initial vector
   @param len  The length of the vector (in octets)
   @param cbc  The CBC state
   @return CRYPT_OK if successful
*/
int cbc_setiv(const unsigned char *IV, unsigned long len, symmetric_CBC *cbc)
{
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(cbc != NULL);
   if (len != (unsigned long)cbc->blocklen) {
      return CRYPT_INVALID_ARG;
   }
   XMEMCPY(cbc->IV, IV, len);
   return CRYPT_OK;
}

#endif 


/* $Source: /cvs/libtom/libtomcrypt/src/modes/cbc/cbc_setiv.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */
