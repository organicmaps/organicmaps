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

#ifdef LTC_RNG_MAKE_PRNG
/**
  @file rng_make_prng.c
  portable way to get secure random bits to feed a PRNG  (Tom St Denis)
*/

/**
  Create a PRNG from a RNG
  @param bits     Number of bits of entropy desired (64 ... 1024)
  @param wprng    Index of which PRNG to setup
  @param prng     [out] PRNG state to initialize
  @param callback A pointer to a void function for when the RNG is slow, this can be NULL
  @return CRYPT_OK if successful
*/
int rng_make_prng(int bits, int wprng, prng_state *prng,
                  void (*callback)(void))
{
   unsigned char buf[256];
   int err;

   LTC_ARGCHK(prng != NULL);

   /* check parameter */
   if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
      return err;
   }

   if (bits < 64 || bits > 1024) {
      return CRYPT_INVALID_PRNGSIZE;
   }

   if ((err = prng_descriptor[wprng].start(prng)) != CRYPT_OK) {
      return err;
   }

   bits = ((bits/8)+((bits&7)!=0?1:0)) * 2;
   if (rng_get_bytes(buf, (unsigned long)bits, callback) != (unsigned long)bits) {
      return CRYPT_ERROR_READPRNG;
   }

   if ((err = prng_descriptor[wprng].add_entropy(buf, (unsigned long)bits, prng)) != CRYPT_OK) {
      return err;
   }

   if ((err = prng_descriptor[wprng].ready(prng)) != CRYPT_OK) {
      return err;
   }

   #ifdef LTC_CLEAN_STACK
      zeromem(buf, sizeof(buf));
   #endif
   return CRYPT_OK;
}
#endif /* #ifdef LTC_RNG_MAKE_PRNG */


/* $Source$ */
/* $Revision$ */
/* $Date$ */
