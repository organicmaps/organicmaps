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
  @file f8_encrypt.c
  F8 implementation, encrypt data, Tom St Denis
*/

#ifdef LTC_F8_MODE

/**
  F8 encrypt
  @param pt     Plaintext
  @param ct     [out] Ciphertext
  @param len    Length of plaintext (octets)
  @param f8     F8 state
  @return CRYPT_OK if successful
*/
int f8_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_F8 *f8)
{
   int           err, x;
   unsigned char buf[MAXBLOCKSIZE];
   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(f8 != NULL);
   if ((err = cipher_is_valid(f8->cipher)) != CRYPT_OK) {
       return err;
   }
   
   /* is blocklen/padlen valid? */
   if (f8->blocklen < 0 || f8->blocklen > (int)sizeof(f8->IV) ||
       f8->padlen   < 0 || f8->padlen   > (int)sizeof(f8->IV)) {
      return CRYPT_INVALID_ARG;
   }
   
   zeromem(buf, sizeof(buf));

   /* make sure the pad is empty */
   if (f8->padlen == f8->blocklen) {
      /* xor of IV, MIV and blockcnt == what goes into cipher */
      STORE32H(f8->blockcnt, (buf+(f8->blocklen-4)));
      ++(f8->blockcnt);
      for (x = 0; x < f8->blocklen; x++) {
          f8->IV[x] ^= f8->MIV[x] ^ buf[x];
      }
      if ((err = cipher_descriptor[f8->cipher].ecb_encrypt(f8->IV, f8->IV, &f8->key)) != CRYPT_OK) {
         return err;
      }
      f8->padlen = 0;
   }

#ifdef LTC_FAST
   if (f8->padlen == 0) {
      while (len >= (unsigned long)f8->blocklen) {
         STORE32H(f8->blockcnt, (buf+(f8->blocklen-4)));
         ++(f8->blockcnt);
         for (x = 0; x < f8->blocklen; x += sizeof(LTC_FAST_TYPE)) {
             *((LTC_FAST_TYPE*)(&ct[x])) = *((LTC_FAST_TYPE*)(&pt[x])) ^ *((LTC_FAST_TYPE*)(&f8->IV[x]));
             *((LTC_FAST_TYPE*)(&f8->IV[x])) ^= *((LTC_FAST_TYPE*)(&f8->MIV[x])) ^ *((LTC_FAST_TYPE*)(&buf[x]));
         }
         if ((err = cipher_descriptor[f8->cipher].ecb_encrypt(f8->IV, f8->IV, &f8->key)) != CRYPT_OK) {
            return err;
         }
         len -= x;
         pt  += x;
         ct  += x;
      }
   }
#endif             

   while (len > 0) {
       if (f8->padlen == f8->blocklen) {
          /* xor of IV, MIV and blockcnt == what goes into cipher */
          STORE32H(f8->blockcnt, (buf+(f8->blocklen-4)));
          ++(f8->blockcnt);
          for (x = 0; x < f8->blocklen; x++) {
              f8->IV[x] ^= f8->MIV[x] ^ buf[x];
          }
          if ((err = cipher_descriptor[f8->cipher].ecb_encrypt(f8->IV, f8->IV, &f8->key)) != CRYPT_OK) {
             return err;
          }
          f8->padlen = 0;
       }
       *ct++ = *pt++ ^ f8->IV[f8->padlen++];
       --len;
   }
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/f8/f8_encrypt.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
