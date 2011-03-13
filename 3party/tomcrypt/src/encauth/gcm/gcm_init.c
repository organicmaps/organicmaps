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
   @file gcm_init.c
   GCM implementation, initialize state, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_GCM_MODE

/**
  Initialize a GCM state
  @param gcm     The GCM state to initialize
  @param cipher  The index of the cipher to use
  @param key     The secret key
  @param keylen  The length of the secret key
  @return CRYPT_OK on success
 */
int gcm_init(gcm_state *gcm, int cipher, 
             const unsigned char *key,  int keylen)
{
   int           err;
   unsigned char B[16];
#ifdef LTC_GCM_TABLES
   int           x, y, z, t;
#endif

   LTC_ARGCHK(gcm != NULL);
   LTC_ARGCHK(key != NULL);

#ifdef LTC_FAST
   if (16 % sizeof(LTC_FAST_TYPE)) {
      return CRYPT_INVALID_ARG;
   }
#endif

   /* is cipher valid? */
   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }
   if (cipher_descriptor[cipher].block_length != 16) {
      return CRYPT_INVALID_CIPHER;
   }

   /* schedule key */
   if ((err = cipher_descriptor[cipher].setup(key, keylen, 0, &gcm->K)) != CRYPT_OK) {
      return err;
   }

   /* H = E(0) */
   zeromem(B, 16);
   if ((err = cipher_descriptor[cipher].ecb_encrypt(B, gcm->H, &gcm->K)) != CRYPT_OK) {
      return err;
   }

   /* setup state */
   zeromem(gcm->buf, sizeof(gcm->buf));
   zeromem(gcm->X,   sizeof(gcm->X));
   gcm->cipher   = cipher;
   gcm->mode     = LTC_GCM_MODE_IV;
   gcm->ivmode   = 0;
   gcm->buflen   = 0;
   gcm->totlen   = 0;
   gcm->pttotlen = 0;

#ifdef LTC_GCM_TABLES
   /* setup tables */

   /* generate the first table as it has no shifting (from which we make the other tables) */
   zeromem(B, 16);
   for (y = 0; y < 256; y++) {
        B[0] = y;
        gcm_gf_mult(gcm->H, B, &gcm->PC[0][y][0]);
   }

   /* now generate the rest of the tables based the previous table */
   for (x = 1; x < 16; x++) {
      for (y = 0; y < 256; y++) {
         /* now shift it right by 8 bits */
         t = gcm->PC[x-1][y][15];
         for (z = 15; z > 0; z--) {
             gcm->PC[x][y][z] = gcm->PC[x-1][y][z-1];
         }
         gcm->PC[x][y][0] = gcm_shift_table[t<<1];
         gcm->PC[x][y][1] ^= gcm_shift_table[(t<<1)+1];
     }
  }

#endif

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_init.c,v $ */
/* $Revision: 1.20 $ */
/* $Date: 2007/05/12 14:32:35 $ */
