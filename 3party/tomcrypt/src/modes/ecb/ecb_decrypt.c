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
  @file ecb_decrypt.c
  ECB implementation, decrypt a block, Tom St Denis
*/

#ifdef LTC_ECB_MODE

/**
  ECB decrypt
  @param ct     Ciphertext
  @param pt     [out] Plaintext
  @param len    The number of octets to process (must be multiple of the cipher block size)
  @param ecb    ECB state
  @return CRYPT_OK if successful
*/
int ecb_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_ECB *ecb)
{
   int err;
   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(ecb != NULL);
   if ((err = cipher_is_valid(ecb->cipher)) != CRYPT_OK) {
       return err;
   }
   if (len % cipher_descriptor[ecb->cipher].block_length) {
      return CRYPT_INVALID_ARG;
   }

   /* check for accel */
   if (cipher_descriptor[ecb->cipher].accel_ecb_decrypt != NULL) {
      return cipher_descriptor[ecb->cipher].accel_ecb_decrypt(ct, pt, len / cipher_descriptor[ecb->cipher].block_length, &ecb->key);
   } else {
      while (len) {
         if ((err = cipher_descriptor[ecb->cipher].ecb_decrypt(ct, pt, &ecb->key)) != CRYPT_OK) {
            return err;
         }
         pt  += cipher_descriptor[ecb->cipher].block_length;
         ct  += cipher_descriptor[ecb->cipher].block_length;
         len -= cipher_descriptor[ecb->cipher].block_length;
      }
   }
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/ecb/ecb_decrypt.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2006/12/28 01:27:24 $ */
