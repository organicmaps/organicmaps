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
   @file ocb_decrypt.c
   OCB implementation, decrypt data, by Tom St Denis 
*/
#include "tomcrypt.h"

#ifdef LTC_OCB_MODE

/**
  Decrypt a block with OCB.
  @param ocb    The OCB state
  @param ct     The ciphertext (length of the block size of the block cipher)
  @param pt     [out] The plaintext (length of ct)
  @return CRYPT_OK if successful
*/
int ocb_decrypt(ocb_state *ocb, const unsigned char *ct, unsigned char *pt)
{
   unsigned char Z[MAXBLOCKSIZE], tmp[MAXBLOCKSIZE];
   int err, x;

   LTC_ARGCHK(ocb != NULL);
   LTC_ARGCHK(pt  != NULL);
   LTC_ARGCHK(ct  != NULL);

   /* check if valid cipher */
   if ((err = cipher_is_valid(ocb->cipher)) != CRYPT_OK) {
      return err;
   }
   LTC_ARGCHK(cipher_descriptor[ocb->cipher].ecb_decrypt != NULL);
   
   /* check length */
   if (ocb->block_len != cipher_descriptor[ocb->cipher].block_length) {
      return CRYPT_INVALID_ARG;
   }

   /* Get Z[i] value */
   ocb_shift_xor(ocb, Z);

   /* xor ct in, encrypt, xor Z out */
   for (x = 0; x < ocb->block_len; x++) {
       tmp[x] = ct[x] ^ Z[x];
   }
   if ((err = cipher_descriptor[ocb->cipher].ecb_decrypt(tmp, pt, &ocb->key)) != CRYPT_OK) {
      return err;
   }
   for (x = 0; x < ocb->block_len; x++) {
       pt[x] ^= Z[x];
   }

   /* compute checksum */
   for (x = 0; x < ocb->block_len; x++) {
       ocb->checksum[x] ^= pt[x];
   }


#ifdef LTC_CLEAN_STACK
   zeromem(Z, sizeof(Z));
   zeromem(tmp, sizeof(tmp));
#endif
   return CRYPT_OK;
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */
