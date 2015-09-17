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
  @file f9_process.c
  f9 Support, process blocks with f9
*/

#ifdef LTC_F9_MODE

/** Process data through f9-MAC
  @param f9       The f9-MAC state
  @param in       Input data to process
  @param inlen    Length of input in octets
  Return CRYPT_OK on success
*/
int f9_process(f9_state *f9, const unsigned char *in, unsigned long inlen)
{
   int err, x;

   LTC_ARGCHK(f9 != NULL);
   LTC_ARGCHK(in   != NULL);

   /* check structure */
   if ((err = cipher_is_valid(f9->cipher)) != CRYPT_OK) {
      return err;
   }

   if ((f9->blocksize > cipher_descriptor[f9->cipher].block_length) || (f9->blocksize < 0) ||
       (f9->buflen > f9->blocksize) || (f9->buflen < 0)) {
      return CRYPT_INVALID_ARG;
   }

#ifdef LTC_FAST
   if (f9->buflen == 0) {
       while (inlen >= (unsigned long)f9->blocksize) {
           for (x = 0; x < f9->blocksize; x += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)&(f9->IV[x])) ^= *((LTC_FAST_TYPE*)&(in[x]));
           }
           cipher_descriptor[f9->cipher].ecb_encrypt(f9->IV, f9->IV, &f9->key);
           for (x = 0; x < f9->blocksize; x += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)&(f9->ACC[x])) ^= *((LTC_FAST_TYPE*)&(f9->IV[x]));
           }
           in    += f9->blocksize;
           inlen -= f9->blocksize;
       }
  }
#endif

   while (inlen) {
     if (f9->buflen == f9->blocksize) {
         cipher_descriptor[f9->cipher].ecb_encrypt(f9->IV, f9->IV, &f9->key);
         for (x = 0; x < f9->blocksize; x++) {
            f9->ACC[x] ^= f9->IV[x];
         }
         f9->buflen = 0;
     }
     f9->IV[f9->buflen++] ^= *in++;
     --inlen;
  }
  return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */

