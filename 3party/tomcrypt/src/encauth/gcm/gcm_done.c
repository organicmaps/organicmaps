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
   @file gcm_done.c
   GCM implementation, Terminate the stream, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_GCM_MODE

/**
  Terminate a GCM stream
  @param gcm     The GCM state
  @param tag     [out] The destination for the MAC tag
  @param taglen  [in/out]  The length of the MAC tag
  @return CRYPT_OK on success
 */
int gcm_done(gcm_state *gcm, 
                     unsigned char *tag,    unsigned long *taglen)
{
   unsigned long x;
   int err;

   LTC_ARGCHK(gcm     != NULL);
   LTC_ARGCHK(tag     != NULL);
   LTC_ARGCHK(taglen  != NULL);

   if (gcm->buflen > 16 || gcm->buflen < 0) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_is_valid(gcm->cipher)) != CRYPT_OK) {
      return err;
   }


   if (gcm->mode != LTC_GCM_MODE_TEXT) {
      return CRYPT_INVALID_ARG;
   }

   /* handle remaining ciphertext */
   if (gcm->buflen) {
      gcm->pttotlen += gcm->buflen * CONST64(8);
      gcm_mult_h(gcm, gcm->X);
   }

   /* length */
   STORE64H(gcm->totlen, gcm->buf);
   STORE64H(gcm->pttotlen, gcm->buf+8);
   for (x = 0; x < 16; x++) {
       gcm->X[x] ^= gcm->buf[x];
   }
   gcm_mult_h(gcm, gcm->X);

   /* encrypt original counter */
   if ((err = cipher_descriptor[gcm->cipher].ecb_encrypt(gcm->Y_0, gcm->buf, &gcm->K)) != CRYPT_OK) {
      return err;
   }
   for (x = 0; x < 16 && x < *taglen; x++) {
       tag[x] = gcm->buf[x] ^ gcm->X[x];
   }
   *taglen = x;

   cipher_descriptor[gcm->cipher].done(&gcm->K);

   return CRYPT_OK;
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_done.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */
