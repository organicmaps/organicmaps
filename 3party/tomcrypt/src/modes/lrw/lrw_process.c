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
   @file lrw_process.c
   LRW_MODE implementation, Encrypt/decrypt blocks, Tom St Denis
*/

#ifdef LTC_LRW_MODE

/**
  Process blocks with LRW, since decrypt/encrypt are largely the same they share this code.
  @param pt        The "input" data
  @param ct        [out] The "output" data
  @param len       The length of the input, must be a multiple of 128-bits (16 octets)
  @param mode      LRW_ENCRYPT or LRW_DECRYPT
  @param lrw       The LRW state
  @return  CRYPT_OK if successful
*/
int lrw_process(const unsigned char *pt, unsigned char *ct, unsigned long len, int mode, symmetric_LRW *lrw)
{
   unsigned char prod[16];
   int           x, err;
#ifdef LTC_LRW_TABLES
   int           y;
#endif

   LTC_ARGCHK(pt  != NULL);
   LTC_ARGCHK(ct  != NULL);
   LTC_ARGCHK(lrw != NULL);

   if (len & 15) {
      return CRYPT_INVALID_ARG;
   }

   while (len) {
      /* copy pad */
      XMEMCPY(prod, lrw->pad, 16);

      /* increment IV */
      for (x = 15; x >= 0; x--) {
          lrw->IV[x] = (lrw->IV[x] + 1) & 255;
          if (lrw->IV[x]) {
              break;
          }
      }

      /* update pad */
#ifdef LTC_LRW_TABLES
      /* for each byte changed we undo it's affect on the pad then add the new product */
      for (; x < 16; x++) {
#ifdef LTC_FAST
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE *)(lrw->pad + y)) ^= *((LTC_FAST_TYPE *)(&lrw->PC[x][lrw->IV[x]][y])) ^ *((LTC_FAST_TYPE *)(&lrw->PC[x][(lrw->IV[x]-1)&255][y]));
          }
#else
          for (y = 0; y < 16; y++) {
              lrw->pad[y] ^= lrw->PC[x][lrw->IV[x]][y] ^ lrw->PC[x][(lrw->IV[x]-1)&255][y];
          }
#endif
      }
#else
      gcm_gf_mult(lrw->tweak, lrw->IV, lrw->pad);
#endif

      /* xor prod */
#ifdef LTC_FAST
      for (x = 0; x < 16; x += sizeof(LTC_FAST_TYPE)) {
           *((LTC_FAST_TYPE *)(ct + x)) = *((LTC_FAST_TYPE *)(pt + x)) ^ *((LTC_FAST_TYPE *)(prod + x));
      }
#else
      for (x = 0; x < 16; x++) {
         ct[x] = pt[x] ^ prod[x];
      }
#endif

      /* send through cipher */
      if (mode == LRW_ENCRYPT) {
         if ((err = cipher_descriptor[lrw->cipher].ecb_encrypt(ct, ct, &lrw->key)) != CRYPT_OK) {
            return err;
         }
      } else {
         if ((err = cipher_descriptor[lrw->cipher].ecb_decrypt(ct, ct, &lrw->key)) != CRYPT_OK) {
            return err;
         }
      }

      /* xor prod */
#ifdef LTC_FAST
      for (x = 0; x < 16; x += sizeof(LTC_FAST_TYPE)) {
           *((LTC_FAST_TYPE *)(ct + x)) = *((LTC_FAST_TYPE *)(ct + x)) ^ *((LTC_FAST_TYPE *)(prod + x));
      }
#else
      for (x = 0; x < 16; x++) {
         ct[x] = ct[x] ^ prod[x];
      }
#endif

      /* move to next */
      pt  += 16;
      ct  += 16;
      len -= 16;
   }

   return CRYPT_OK;
}

#endif
/* $Source$ */
/* $Revision$ */
/* $Date$ */
