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
   @file gcm_add_aad.c
   GCM implementation, Add AAD data to the stream, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_GCM_MODE

/**
  Add AAD to the GCM state
  @param gcm       The GCM state
  @param adata     The additional authentication data to add to the GCM state
  @param adatalen  The length of the AAD data.
  @return CRYPT_OK on success
 */
int gcm_add_aad(gcm_state *gcm,
               const unsigned char *adata,  unsigned long adatalen)
{
   unsigned long x;
   int           err;
#ifdef LTC_FAST
   unsigned long y;
#endif

   LTC_ARGCHK(gcm    != NULL);
   if (adatalen > 0) {
      LTC_ARGCHK(adata  != NULL);
   }

   if (gcm->buflen > 16 || gcm->buflen < 0) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_is_valid(gcm->cipher)) != CRYPT_OK) {
      return err;
   }

   /* in IV mode? */
   if (gcm->mode == LTC_GCM_MODE_IV) {
      /* let's process the IV */
      if (gcm->ivmode || gcm->buflen != 12) {
         for (x = 0; x < (unsigned long)gcm->buflen; x++) {
             gcm->X[x] ^= gcm->buf[x];
         }
         if (gcm->buflen) {
            gcm->totlen += gcm->buflen * CONST64(8);
            gcm_mult_h(gcm, gcm->X);
         }

         /* mix in the length */
         zeromem(gcm->buf, 8);
         STORE64H(gcm->totlen, gcm->buf+8);
         for (x = 0; x < 16; x++) {
             gcm->X[x] ^= gcm->buf[x];
         }
         gcm_mult_h(gcm, gcm->X);

         /* copy counter out */ 
         XMEMCPY(gcm->Y, gcm->X, 16);
         zeromem(gcm->X, 16);
      } else {
         XMEMCPY(gcm->Y, gcm->buf, 12);
         gcm->Y[12] = 0;
         gcm->Y[13] = 0;
         gcm->Y[14] = 0;
         gcm->Y[15] = 1;
      }
      XMEMCPY(gcm->Y_0, gcm->Y, 16);
      zeromem(gcm->buf, 16);
      gcm->buflen = 0;
      gcm->totlen = 0;
      gcm->mode   = LTC_GCM_MODE_AAD;
   }

   if (gcm->mode != LTC_GCM_MODE_AAD || gcm->buflen >= 16) {
      return CRYPT_INVALID_ARG;
   }

   x = 0;
#ifdef LTC_FAST
   if (gcm->buflen == 0) {
      for (x = 0; x < (adatalen & ~15); x += 16) {
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&gcm->X[y])) ^= *((LTC_FAST_TYPE*)(&adata[x + y]));
          }
          gcm_mult_h(gcm, gcm->X);
          gcm->totlen += 128;
      }
      adata += x;
   }
#endif


   /* start adding AAD data to the state */
   for (; x < adatalen; x++) {
       gcm->X[gcm->buflen++] ^= *adata++;

       if (gcm->buflen == 16) {
         /* GF mult it */
         gcm_mult_h(gcm, gcm->X);
         gcm->buflen = 0;
         gcm->totlen += 128;
      }
   }

   return CRYPT_OK;
}
#endif
   

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_add_aad.c,v $ */
/* $Revision: 1.18 $ */
/* $Date: 2007/05/12 14:32:35 $ */
