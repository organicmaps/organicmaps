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
   @file lrw_setiv.c
   LRW_MODE implementation, Set the current IV, Tom St Denis
*/

#ifdef LTC_LRW_MODE

/**
  Set the IV for LRW
  @param IV      The IV, must be 16 octets
  @param len     Length ... must be 16 :-)
  @param lrw     The LRW state to update
  @return CRYPT_OK if successful
*/
int lrw_setiv(const unsigned char *IV, unsigned long len, symmetric_LRW *lrw)
{
   int           err;
#ifdef LRW_TABLES
   unsigned char T[16];
   int           x, y;
#endif
   LTC_ARGCHK(IV != NULL);
   LTC_ARGCHK(lrw != NULL);

   if (len != 16) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_is_valid(lrw->cipher)) != CRYPT_OK) {
      return err;
   }

   /* copy the IV */
   XMEMCPY(lrw->IV, IV, 16);

   /* check if we have to actually do work */
   if (cipher_descriptor[lrw->cipher].accel_lrw_encrypt != NULL && cipher_descriptor[lrw->cipher].accel_lrw_decrypt != NULL) {
       /* we have accelerators, let's bail since they don't use lrw->pad anyways */
       return CRYPT_OK;
   }

#ifdef LRW_TABLES
   XMEMCPY(T, &lrw->PC[0][IV[0]][0], 16);
   for (x = 1; x < 16; x++) {
#ifdef LTC_FAST
       for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
           *((LTC_FAST_TYPE *)(T + y)) ^= *((LTC_FAST_TYPE *)(&lrw->PC[x][IV[x]][y]));
       }
#else
       for (y = 0; y < 16; y++) {
           T[y] ^= lrw->PC[x][IV[x]][y];
       }
#endif
   }
   XMEMCPY(lrw->pad, T, 16);
#else     
   gcm_gf_mult(lrw->tweak, IV, lrw->pad); 
#endif

   return CRYPT_OK;
}


#endif
/* $Source: /cvs/libtom/libtomcrypt/src/modes/lrw/lrw_setiv.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2006/12/28 01:27:24 $ */
