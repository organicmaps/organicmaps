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
   @file pmac_process.c
   PMAC implementation, process data, by Tom St Denis 
*/


#ifdef LTC_PMAC

/**
  Process data in a PMAC stream
  @param pmac     The PMAC state
  @param in       The data to send through PMAC
  @param inlen    The length of the data to send through PMAC
  @return CRYPT_OK if successful
*/
int pmac_process(pmac_state *pmac, const unsigned char *in, unsigned long inlen)
{
   int err, n;
   unsigned long x;
   unsigned char Z[MAXBLOCKSIZE];

   LTC_ARGCHK(pmac != NULL);
   LTC_ARGCHK(in   != NULL);
   if ((err = cipher_is_valid(pmac->cipher_idx)) != CRYPT_OK) {
      return err;
   }

   if ((pmac->buflen > (int)sizeof(pmac->block)) || (pmac->buflen < 0) ||
       (pmac->block_len > (int)sizeof(pmac->block)) || (pmac->buflen > pmac->block_len)) {
      return CRYPT_INVALID_ARG;
   }

#ifdef LTC_FAST
   if (pmac->buflen == 0 && inlen > 16) {
      unsigned long y;
      for (x = 0; x < (inlen - 16); x += 16) {
          pmac_shift_xor(pmac);
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&Z[y])) = *((LTC_FAST_TYPE*)(&in[y])) ^ *((LTC_FAST_TYPE*)(&pmac->Li[y]));
          }
          if ((err = cipher_descriptor[pmac->cipher_idx].ecb_encrypt(Z, Z, &pmac->key)) != CRYPT_OK) {
             return err;
          }
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&pmac->checksum[y])) ^= *((LTC_FAST_TYPE*)(&Z[y]));
          }
          in += 16;
      }
      inlen -= x;
   }
#endif

   while (inlen != 0) { 
       /* ok if the block is full we xor in prev, encrypt and replace prev */
       if (pmac->buflen == pmac->block_len) {
          pmac_shift_xor(pmac);
          for (x = 0; x < (unsigned long)pmac->block_len; x++) {
               Z[x] = pmac->Li[x] ^ pmac->block[x];
          }
          if ((err = cipher_descriptor[pmac->cipher_idx].ecb_encrypt(Z, Z, &pmac->key)) != CRYPT_OK) {
             return err;
           }
          for (x = 0; x < (unsigned long)pmac->block_len; x++) {
              pmac->checksum[x] ^= Z[x];
          }
          pmac->buflen = 0;
       }

       /* add bytes */
       n = MIN(inlen, (unsigned long)(pmac->block_len - pmac->buflen));
       XMEMCPY(pmac->block + pmac->buflen, in, n);
       pmac->buflen  += n;
       inlen         -= n;
       in            += n;
   }

#ifdef LTC_CLEAN_STACK
   zeromem(Z, sizeof(Z));
#endif

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/mac/pmac/pmac_process.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2006/12/28 01:27:23 $ */
