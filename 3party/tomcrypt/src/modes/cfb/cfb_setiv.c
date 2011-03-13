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
  @file cfb_setiv.c
  CFB implementation, set IV, Tom St Denis
*/  

#ifdef LTC_CFB_MODE

/**
   Set an initial vector
   @param IV   The initial vector
   @param len  The length of the vector (in octets)
   @param cfb  The CFB state
   @return CRYPT_OK if successful
*/
int cfb_setiv(const unsigned char *IV, unsigned long len, symmetric_CFB *cfb)
{
   int err;
   
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(cfb != NULL);

   if ((err = cipher_is_valid(cfb->cipher)) != CRYPT_OK) {
       return err;
   }
   
   if (len != (unsigned long)cfb->blocklen) {
      return CRYPT_INVALID_ARG;
   }
      
   /* force next block */
   cfb->padlen = 0;
   return cipher_descriptor[cfb->cipher].ecb_encrypt(IV, cfb->IV, &cfb->key);
}

#endif 


/* $Source: /cvs/libtom/libtomcrypt/src/modes/cfb/cfb_setiv.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
