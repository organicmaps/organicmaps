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
   @file lrw_decrypt.c
   LRW_MODE implementation, Decrypt blocks, Tom St Denis
*/

#ifdef LTC_LRW_MODE

/**
  LRW decrypt blocks
  @param ct     The ciphertext
  @param pt     [out] The plaintext
  @param len    The length in octets, must be a multiple of 16
  @param lrw    The LRW state
*/
int lrw_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_LRW *lrw)
{
   int err;

   LTC_ARGCHK(pt  != NULL);
   LTC_ARGCHK(ct  != NULL);
   LTC_ARGCHK(lrw != NULL);

   if ((err = cipher_is_valid(lrw->cipher)) != CRYPT_OK) {
      return err;
   }

   if (cipher_descriptor[lrw->cipher].accel_lrw_decrypt != NULL) {
      return cipher_descriptor[lrw->cipher].accel_lrw_decrypt(ct, pt, len, lrw->IV, lrw->tweak, &lrw->key);
   }

   return lrw_process(ct, pt, len, LRW_DECRYPT, lrw);
}


#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/lrw/lrw_decrypt.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2006/12/28 01:27:24 $ */
