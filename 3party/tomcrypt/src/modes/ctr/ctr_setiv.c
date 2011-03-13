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
  @file ctr_setiv.c
  CTR implementation, set IV, Tom St Denis
*/
  
#ifdef LTC_CTR_MODE

/**
   Set an initial vector
   @param IV   The initial vector
   @param len  The length of the vector (in octets)
   @param ctr  The CTR state
   @return CRYPT_OK if successful
*/
int ctr_setiv(const unsigned char *IV, unsigned long len, symmetric_CTR *ctr)
{
   int err;
   
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(ctr != NULL);

   /* bad param? */
   if ((err = cipher_is_valid(ctr->cipher)) != CRYPT_OK) {
      return err;
   }
   
   if (len != (unsigned long)ctr->blocklen) {
      return CRYPT_INVALID_ARG;
   }

   /* set IV */
   XMEMCPY(ctr->ctr, IV, len);
   
   /* force next block */
   ctr->padlen = 0;
   return cipher_descriptor[ctr->cipher].ecb_encrypt(IV, ctr->pad, &ctr->key);
}

#endif 


/* $Source: /cvs/libtom/libtomcrypt/src/modes/ctr/ctr_setiv.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
