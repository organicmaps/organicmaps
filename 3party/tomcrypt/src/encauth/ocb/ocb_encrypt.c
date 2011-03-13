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

/** 
   @file ocb_encrypt.c
   OCB implementation, encrypt data, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_OCB_MODE

/**
   Encrypt a block of data with OCB.
   @param ocb     The OCB state
   @param pt      The plaintext (length of the block size of the block cipher)
   @param ct      [out] The ciphertext (same size as the pt)
   @return CRYPT_OK if successful
*/
int ocb_encrypt(ocb_state *ocb, const unsigned char *pt, unsigned char *ct)
{
   unsigned char Z[MAXBLOCKSIZE], tmp[MAXBLOCKSIZE];
   int err, x;

   LTC_ARGCHK(ocb != NULL);
   LTC_ARGCHK(pt  != NULL);
   LTC_ARGCHK(ct  != NULL);
   if ((err = cipher_is_valid(ocb->cipher)) != CRYPT_OK) {
      return err;
   }
   if (ocb->block_len != cipher_descriptor[ocb->cipher].block_length) {
      return CRYPT_INVALID_ARG;
   }

   /* compute checksum */
   for (x = 0; x < ocb->block_len; x++) {
       ocb->checksum[x] ^= pt[x];
   }

   /* Get Z[i] value */
   ocb_shift_xor(ocb, Z);

   /* xor pt in, encrypt, xor Z out */
   for (x = 0; x < ocb->block_len; x++) {
       tmp[x] = pt[x] ^ Z[x];
   }
   if ((err = cipher_descriptor[ocb->cipher].ecb_encrypt(tmp, ct, &ocb->key)) != CRYPT_OK) {
      return err;
   }
   for (x = 0; x < ocb->block_len; x++) {
       ct[x] ^= Z[x];
   }

#ifdef LTC_CLEAN_STACK
   zeromem(Z, sizeof(Z));
   zeromem(tmp, sizeof(tmp));
#endif
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/ocb/ocb_encrypt.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */
