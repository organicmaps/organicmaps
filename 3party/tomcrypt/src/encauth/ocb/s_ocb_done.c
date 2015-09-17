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
   @file s_ocb_done.c
   OCB implementation, internal helper, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_OCB_MODE

/* Since the last block is encrypted in CTR mode the same code can
 * be used to finish a decrypt or encrypt stream.  The only difference
 * is we XOR the final ciphertext into the checksum so we have to xor it
 * before we CTR [decrypt] or after [encrypt]
 *
 * the names pt/ptlen/ct really just mean in/inlen/out but this is the way I wrote it... 
 */

/**
   Shared code to finish an OCB stream
   @param ocb    The OCB state
   @param pt     The remaining plaintext [or input]
   @param ptlen  The length of the input (octets)
   @param ct     [out] The output buffer
   @param tag    [out] The destination for the authentication tag
   @param taglen [in/out] The max size and resulting size of the authentication tag
   @param mode   The mode we are terminating, 0==encrypt, 1==decrypt
   @return       CRYPT_OK if successful
*/
int s_ocb_done(ocb_state *ocb, const unsigned char *pt, unsigned long ptlen,
               unsigned char *ct, unsigned char *tag, unsigned long *taglen, int mode)

{
   unsigned char *Z, *Y, *X;
   int err, x;

   LTC_ARGCHK(ocb    != NULL);
   LTC_ARGCHK(pt     != NULL);
   LTC_ARGCHK(ct     != NULL);
   LTC_ARGCHK(tag    != NULL);
   LTC_ARGCHK(taglen != NULL);
   if ((err = cipher_is_valid(ocb->cipher)) != CRYPT_OK) {
      return err;
   }
   if (ocb->block_len != cipher_descriptor[ocb->cipher].block_length ||
       (int)ptlen > ocb->block_len || (int)ptlen < 0) {
      return CRYPT_INVALID_ARG;
   }

   /* allocate ram */
   Z = XMALLOC(MAXBLOCKSIZE);
   Y = XMALLOC(MAXBLOCKSIZE);
   X = XMALLOC(MAXBLOCKSIZE);
   if (X == NULL || Y == NULL || Z == NULL) {
      if (X != NULL) {
         XFREE(X);
      }
      if (Y != NULL) {
         XFREE(Y);
      }
      if (Z != NULL) {
         XFREE(Z);
      }
      return CRYPT_MEM;
   }

   /* compute X[m] = len(pt[m]) XOR Lr XOR Z[m] */
   ocb_shift_xor(ocb, X); 
   XMEMCPY(Z, X, ocb->block_len);

   X[ocb->block_len-1] ^= (ptlen*8)&255;
   X[ocb->block_len-2] ^= ((ptlen*8)>>8)&255;
   for (x = 0; x < ocb->block_len; x++) {
       X[x] ^= ocb->Lr[x]; 
   }

   /* Y[m] = E(X[m])) */
   if ((err = cipher_descriptor[ocb->cipher].ecb_encrypt(X, Y, &ocb->key)) != CRYPT_OK) {
      goto error;
   }

   if (mode == 1) {
      /* decrypt mode, so let's xor it first */
      /* xor C[m] into checksum */
      for (x = 0; x < (int)ptlen; x++) {
         ocb->checksum[x] ^= ct[x];
      }  
   }

   /* C[m] = P[m] xor Y[m] */
   for (x = 0; x < (int)ptlen; x++) {
       ct[x] = pt[x] ^ Y[x];
   }

   if (mode == 0) {
      /* encrypt mode */    
      /* xor C[m] into checksum */
      for (x = 0; x < (int)ptlen; x++) {
          ocb->checksum[x] ^= ct[x];
      }
   }

   /* xor Y[m] and Z[m] into checksum */
   for (x = 0; x < ocb->block_len; x++) {
       ocb->checksum[x] ^= Y[x] ^ Z[x];
   }
   
   /* encrypt checksum, er... tag!! */
   if ((err = cipher_descriptor[ocb->cipher].ecb_encrypt(ocb->checksum, X, &ocb->key)) != CRYPT_OK) {
      goto error;
   }
   cipher_descriptor[ocb->cipher].done(&ocb->key);

   /* now store it */
   for (x = 0; x < ocb->block_len && x < (int)*taglen; x++) {
       tag[x] = X[x];
   }
   *taglen = x;

#ifdef LTC_CLEAN_STACK
   zeromem(X, MAXBLOCKSIZE);
   zeromem(Y, MAXBLOCKSIZE);
   zeromem(Z, MAXBLOCKSIZE);
   zeromem(ocb, sizeof(*ocb));
#endif
error:   
   XFREE(X);
   XFREE(Y);
   XFREE(Z);

   return err;
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */
