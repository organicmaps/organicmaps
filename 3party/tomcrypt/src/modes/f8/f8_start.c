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
   @file f8_start.c
   F8 implementation, start chain, Tom St Denis
*/


#ifdef LTC_F8_MODE

/**
   Initialize an F8 context
   @param cipher      The index of the cipher desired
   @param IV          The initial vector
   @param key         The secret key 
   @param keylen      The length of the secret key (octets)
   @param salt_key    The salting key for the IV
   @param skeylen     The length of the salting key (octets)
   @param num_rounds  Number of rounds in the cipher desired (0 for default)
   @param f8          The F8 state to initialize
   @return CRYPT_OK if successful
*/
int f8_start(                int  cipher, const unsigned char *IV, 
             const unsigned char *key,                    int  keylen, 
             const unsigned char *salt_key,               int  skeylen,
                             int  num_rounds,   symmetric_F8  *f8)
{
   int           x, err;
   unsigned char tkey[MAXBLOCKSIZE];

   LTC_ARGCHK(IV       != NULL);
   LTC_ARGCHK(key      != NULL);
   LTC_ARGCHK(salt_key != NULL);
   LTC_ARGCHK(f8       != NULL);

   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

#ifdef LTC_FAST
   if (cipher_descriptor[cipher].block_length % sizeof(LTC_FAST_TYPE)) {
      return CRYPT_INVALID_ARG;
   }
#endif

   /* copy details */
   f8->blockcnt = 0;
   f8->cipher   = cipher;
   f8->blocklen = cipher_descriptor[cipher].block_length;
   f8->padlen   = f8->blocklen;
   
   /* now get key ^ salt_key [extend salt_ket with 0x55 as required to match length] */
   zeromem(tkey, sizeof(tkey));
   for (x = 0; x < keylen && x < (int)sizeof(tkey); x++) {
       tkey[x] = key[x];
   }
   for (x = 0; x < skeylen && x < (int)sizeof(tkey); x++) {
       tkey[x] ^= salt_key[x];
   }       
   for (; x < keylen && x < (int)sizeof(tkey); x++) {
       tkey[x] ^= 0x55;
   }
   
   /* now encrypt with tkey[0..keylen-1] the IV and use that as the IV */
   if ((err = cipher_descriptor[cipher].setup(tkey, keylen, num_rounds, &f8->key)) != CRYPT_OK) {
      return err;
   }
   
   /* encrypt IV */
   if ((err = cipher_descriptor[f8->cipher].ecb_encrypt(IV, f8->MIV, &f8->key)) != CRYPT_OK) {
      cipher_descriptor[f8->cipher].done(&f8->key);
      return err;
   }
   zeromem(tkey, sizeof(tkey));
   zeromem(f8->IV, sizeof(f8->IV));
   
   /* terminate this cipher */
   cipher_descriptor[f8->cipher].done(&f8->key);
   
   /* init the cipher */
   return cipher_descriptor[cipher].setup(key, keylen, num_rounds, &f8->key);
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/modes/f8/f8_start.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2006/12/28 01:27:24 $ */
