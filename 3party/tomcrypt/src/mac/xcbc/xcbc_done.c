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
  @file xcbc_done.c
  XCBC Support, terminate the state
*/

#ifdef LTC_XCBC

/** Terminate the XCBC-MAC state
  @param xcbc     XCBC state to terminate
  @param out      [out] Destination for the MAC tag
  @param outlen   [in/out] Destination size and final tag size
  Return CRYPT_OK on success
*/
int xcbc_done(xcbc_state *xcbc, unsigned char *out, unsigned long *outlen)
{
   int err, x;
   LTC_ARGCHK(xcbc != NULL);
   LTC_ARGCHK(out  != NULL);

   /* check structure */
   if ((err = cipher_is_valid(xcbc->cipher)) != CRYPT_OK) {
      return err;
   }

   if ((xcbc->blocksize > cipher_descriptor[xcbc->cipher].block_length) || (xcbc->blocksize < 0) ||
       (xcbc->buflen > xcbc->blocksize) || (xcbc->buflen < 0)) {
      return CRYPT_INVALID_ARG;
   }

   /* which key do we use? */
   if (xcbc->buflen == xcbc->blocksize) {
      /* k2 */
      for (x = 0; x < xcbc->blocksize; x++) {
         xcbc->IV[x] ^= xcbc->K[1][x];
      }
   } else {
      xcbc->IV[xcbc->buflen] ^= 0x80;
      /* k3 */
      for (x = 0; x < xcbc->blocksize; x++) {
         xcbc->IV[x] ^= xcbc->K[2][x];
      }
   }

   /* encrypt */
   cipher_descriptor[xcbc->cipher].ecb_encrypt(xcbc->IV, xcbc->IV, &xcbc->key);
   cipher_descriptor[xcbc->cipher].done(&xcbc->key);

   /* extract tag */
   for (x = 0; x < xcbc->blocksize && (unsigned long)x < *outlen; x++) {
      out[x] = xcbc->IV[x];
   }
   *outlen = x;
  
#ifdef LTC_CLEAN_STACK
   zeromem(xcbc, sizeof(*xcbc));
#endif
   return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */

