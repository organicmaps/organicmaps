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
  @file pmac_done.c
  PMAC implementation, terminate a session, by Tom St Denis 
*/

#ifdef LTC_PMAC

int pmac_done(pmac_state *state, unsigned char *out, unsigned long *outlen)
{
   int err, x;

   LTC_ARGCHK(state != NULL);
   LTC_ARGCHK(out   != NULL);
   if ((err = cipher_is_valid(state->cipher_idx)) != CRYPT_OK) {
      return err;
   }

   if ((state->buflen > (int)sizeof(state->block)) || (state->buflen < 0) ||
       (state->block_len > (int)sizeof(state->block)) || (state->buflen > state->block_len)) {
      return CRYPT_INVALID_ARG;
   }


   /* handle padding.  If multiple xor in L/x */

   if (state->buflen == state->block_len) {
      /* xor Lr against the checksum */
      for (x = 0; x < state->block_len; x++) {
          state->checksum[x] ^= state->block[x] ^ state->Lr[x];
      }
   } else {
      /* otherwise xor message bytes then the 0x80 byte */
      for (x = 0; x < state->buflen; x++) {
          state->checksum[x] ^= state->block[x];
      }
      state->checksum[x] ^= 0x80;
   }

   /* encrypt it */
   if ((err = cipher_descriptor[state->cipher_idx].ecb_encrypt(state->checksum, state->checksum, &state->key)) != CRYPT_OK) {
      return err;
   }
   cipher_descriptor[state->cipher_idx].done(&state->key);

   /* store it */
   for (x = 0; x < state->block_len && x < (int)*outlen; x++) {
       out[x] = state->checksum[x];
   }
   *outlen = x;

#ifdef LTC_CLEAN_STACK
   zeromem(state, sizeof(*state));
#endif
   return CRYPT_OK;
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/mac/pmac/pmac_done.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2006/12/28 01:27:23 $ */
