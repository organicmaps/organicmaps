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
   @file lrw_start.c
   LRW_MODE implementation, start mode, Tom St Denis
*/

#ifdef LTC_LRW_MODE

/**
  Initialize the LRW context
  @param cipher        The cipher desired, must be a 128-bit block cipher 
  @param IV            The index value, must be 128-bits
  @param key           The cipher key 
  @param keylen        The length of the cipher key in octets
  @param tweak         The tweak value (second key), must be 128-bits
  @param num_rounds    The number of rounds for the cipher (0 == default)
  @param lrw           [out] The LRW state
  @return CRYPT_OK on success.
*/
int lrw_start(               int   cipher,
              const unsigned char *IV,
              const unsigned char *key,       int keylen,
              const unsigned char *tweak,
                             int  num_rounds, 
                   symmetric_LRW *lrw)
{
   int           err;
#ifdef LRW_TABLES
   unsigned char B[16];
   int           x, y, z, t;
#endif

  LTC_ARGCHK(IV    != NULL);
  LTC_ARGCHK(key   != NULL);
  LTC_ARGCHK(tweak != NULL);
  LTC_ARGCHK(lrw   != NULL);

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
   if ((err = cipher_descriptor[cipher].setup(key, keylen, num_rounds, &lrw->key)) != CRYPT_OK) {
      return err;
   }
   lrw->cipher = cipher;

   /* copy the IV and tweak */
   XMEMCPY(lrw->tweak, tweak, 16);

#ifdef LRW_TABLES
   /* setup tables */
   /* generate the first table as it has no shifting (from which we make the other tables) */
   zeromem(B, 16);
   for (y = 0; y < 256; y++) {
        B[0] = y;
        gcm_gf_mult(tweak, B, &lrw->PC[0][y][0]);
   }

   /* now generate the rest of the tables based the previous table */
   for (x = 1; x < 16; x++) {
      for (y = 0; y < 256; y++) {
         /* now shift it right by 8 bits */
         t = lrw->PC[x-1][y][15];
         for (z = 15; z > 0; z--) {
             lrw->PC[x][y][z] = lrw->PC[x-1][y][z-1];
         }
         lrw->PC[x][y][0]  = gcm_shift_table[t<<1];
         lrw->PC[x][y][1] ^= gcm_shift_table[(t<<1)+1];
     }
  }
#endif

   /* generate first pad */
   return lrw_setiv(IV, 16, lrw);
}


#endif
/* $Source: /cvs/libtom/libtomcrypt/src/modes/lrw/lrw_start.c,v $ */
/* $Revision: 1.12 $ */
/* $Date: 2006/12/28 01:27:24 $ */
