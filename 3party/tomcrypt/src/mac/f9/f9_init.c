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
  @file f9_init.c
  F9 Support, start an F9 state
*/

#ifdef LTC_F9_MODE

/** Initialize F9-MAC state
  @param f9    [out] f9 state to initialize
  @param cipher  Index of cipher to use
  @param key     [in]  Secret key
  @param keylen  Length of secret key in octets
  Return CRYPT_OK on success
*/
int f9_init(f9_state *f9, int cipher, const unsigned char *key, unsigned long keylen)
{
   int            x, err;

   LTC_ARGCHK(f9   != NULL);
   LTC_ARGCHK(key  != NULL);

   /* schedule the key */
   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

#ifdef LTC_FAST
   if (cipher_descriptor[cipher].block_length % sizeof(LTC_FAST_TYPE)) {
       return CRYPT_INVALID_ARG;
   }
#endif

   if ((err = cipher_descriptor[cipher].setup(key, keylen, 0, &f9->key)) != CRYPT_OK) {
      goto done;
   }
   
   /* make the second key */
   for (x = 0; (unsigned)x < keylen; x++) {
      f9->akey[x] = key[x] ^ 0xAA;
   }
 
   /* setup struct */
   zeromem(f9->IV,  cipher_descriptor[cipher].block_length);
   zeromem(f9->ACC, cipher_descriptor[cipher].block_length);
   f9->blocksize = cipher_descriptor[cipher].block_length;
   f9->cipher    = cipher;
   f9->buflen    = 0;
   f9->keylen    = keylen;
done:
   return err;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */

