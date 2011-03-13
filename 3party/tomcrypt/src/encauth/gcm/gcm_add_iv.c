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
   @file gcm_add_iv.c
   GCM implementation, add IV data to the state, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_GCM_MODE

/**
  Add IV data to the GCM state
  @param gcm    The GCM state
  @param IV     The initial value data to add
  @param IVlen  The length of the IV
  @return CRYPT_OK on success
 */
int gcm_add_iv(gcm_state *gcm, 
               const unsigned char *IV,     unsigned long IVlen)
{
   unsigned long x, y;
   int           err;

   LTC_ARGCHK(gcm != NULL);
   if (IVlen > 0) {
      LTC_ARGCHK(IV  != NULL);
   }

   /* must be in IV mode */
   if (gcm->mode != LTC_GCM_MODE_IV) {
      return CRYPT_INVALID_ARG;
   }
 
   if (gcm->buflen >= 16 || gcm->buflen < 0) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_is_valid(gcm->cipher)) != CRYPT_OK) {
      return err;
   }


   /* trip the ivmode flag */
   if (IVlen + gcm->buflen > 12) {
      gcm->ivmode |= 1;
   }

   x = 0;
#ifdef LTC_FAST
   if (gcm->buflen == 0) {
      for (x = 0; x < (IVlen & ~15); x += 16) {
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&gcm->X[y])) ^= *((LTC_FAST_TYPE*)(&IV[x + y]));
          }
          gcm_mult_h(gcm, gcm->X);
          gcm->totlen += 128;
      }
      IV += x;
   }
#endif

   /* start adding IV data to the state */
   for (; x < IVlen; x++) {
       gcm->buf[gcm->buflen++] = *IV++;

       if (gcm->buflen == 16) {
         /* GF mult it */
         for (y = 0; y < 16; y++) {
             gcm->X[y] ^= gcm->buf[y];
         }
         gcm_mult_h(gcm, gcm->X);
         gcm->buflen = 0;
         gcm->totlen += 128;
      }
   }

   return CRYPT_OK;
}

#endif
   

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_add_iv.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2007/05/12 14:32:35 $ */
