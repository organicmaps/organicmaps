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
  @file xtea.c
  Implementation of LTC_XTEA, Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_XTEA

const struct ltc_cipher_descriptor xtea_desc =
{
    "xtea",
    1,
    16, 16, 8, 32,
    &xtea_setup,
    &xtea_ecb_encrypt,
    &xtea_ecb_decrypt,
    &xtea_test,
    &xtea_done,
    &xtea_keysize,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int xtea_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
   unsigned long x, sum, K[4];
   
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(skey != NULL);

   /* check arguments */
   if (keylen != 16) {
      return CRYPT_INVALID_KEYSIZE;
   }

   if (num_rounds != 0 && num_rounds != 32) {
      return CRYPT_INVALID_ROUNDS;
   }

   /* load key */
   LOAD32L(K[0], key+0);
   LOAD32L(K[1], key+4);
   LOAD32L(K[2], key+8);
   LOAD32L(K[3], key+12);
   
   for (x = sum = 0; x < 32; x++) {
       skey->xtea.A[x] = (sum + K[sum&3]) & 0xFFFFFFFFUL;
       sum = (sum + 0x9E3779B9UL) & 0xFFFFFFFFUL;
       skey->xtea.B[x] = (sum + K[(sum>>11)&3]) & 0xFFFFFFFFUL;
   }
   
#ifdef LTC_CLEAN_STACK
   zeromem(&K, sizeof(K));
#endif   
   
   return CRYPT_OK;
}

/**
  Encrypts a block of text with LTC_XTEA
  @param pt The input plaintext (8 bytes)
  @param ct The output ciphertext (8 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
int xtea_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
   unsigned long y, z;
   int r;

   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);

   LOAD32L(y, &pt[0]);
   LOAD32L(z, &pt[4]);
   for (r = 0; r < 32; r += 4) {
       y = (y + ((((z<<4)^(z>>5)) + z) ^ skey->xtea.A[r])) & 0xFFFFFFFFUL;
       z = (z + ((((y<<4)^(y>>5)) + y) ^ skey->xtea.B[r])) & 0xFFFFFFFFUL;

       y = (y + ((((z<<4)^(z>>5)) + z) ^ skey->xtea.A[r+1])) & 0xFFFFFFFFUL;
       z = (z + ((((y<<4)^(y>>5)) + y) ^ skey->xtea.B[r+1])) & 0xFFFFFFFFUL;

       y = (y + ((((z<<4)^(z>>5)) + z) ^ skey->xtea.A[r+2])) & 0xFFFFFFFFUL;
       z = (z + ((((y<<4)^(y>>5)) + y) ^ skey->xtea.B[r+2])) & 0xFFFFFFFFUL;

       y = (y + ((((z<<4)^(z>>5)) + z) ^ skey->xtea.A[r+3])) & 0xFFFFFFFFUL;
       z = (z + ((((y<<4)^(y>>5)) + y) ^ skey->xtea.B[r+3])) & 0xFFFFFFFFUL;
   }
   STORE32L(y, &ct[0]);
   STORE32L(z, &ct[4]);
   return CRYPT_OK;
}

/**
  Decrypts a block of text with LTC_XTEA
  @param ct The input ciphertext (8 bytes)
  @param pt The output plaintext (8 bytes)
  @param skey The key as scheduled 
  @return CRYPT_OK if successful
*/
int xtea_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
   unsigned long y, z;
   int r;

   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);

   LOAD32L(y, &ct[0]);
   LOAD32L(z, &ct[4]);
   for (r = 31; r >= 0; r -= 4) {
       z = (z - ((((y<<4)^(y>>5)) + y) ^ skey->xtea.B[r])) & 0xFFFFFFFFUL;
       y = (y - ((((z<<4)^(z>>5)) + z) ^ skey->xtea.A[r])) & 0xFFFFFFFFUL;

       z = (z - ((((y<<4)^(y>>5)) + y) ^ skey->xtea.B[r-1])) & 0xFFFFFFFFUL;
       y = (y - ((((z<<4)^(z>>5)) + z) ^ skey->xtea.A[r-1])) & 0xFFFFFFFFUL;

       z = (z - ((((y<<4)^(y>>5)) + y) ^ skey->xtea.B[r-2])) & 0xFFFFFFFFUL;
       y = (y - ((((z<<4)^(z>>5)) + z) ^ skey->xtea.A[r-2])) & 0xFFFFFFFFUL;

       z = (z - ((((y<<4)^(y>>5)) + y) ^ skey->xtea.B[r-3])) & 0xFFFFFFFFUL;
       y = (y - ((((z<<4)^(z>>5)) + z) ^ skey->xtea.A[r-3])) & 0xFFFFFFFFUL;
   }
   STORE32L(y, &pt[0]);
   STORE32L(z, &pt[4]);
   return CRYPT_OK;
}

/**
  Performs a self-test of the LTC_XTEA block cipher
  @return CRYPT_OK if functional, CRYPT_NOP if self-test has been disabled
*/
int xtea_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else    
   static const unsigned char key[16] = 
      { 0x78, 0x56, 0x34, 0x12, 0xf0, 0xcd, 0xcb, 0x9a,
        0x48, 0x37, 0x26, 0x15, 0xc0, 0xbf, 0xae, 0x9d };
   static const unsigned char pt[8] = 
      { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
   static const unsigned char ct[8] = 
      { 0x75, 0xd7, 0xc5, 0xbf, 0xcf, 0x58, 0xc9, 0x3f };
   unsigned char tmp[2][8];
   symmetric_key skey;
   int err, y;

   if ((err = xtea_setup(key, 16, 0, &skey)) != CRYPT_OK)  {
      return err;
   }
   xtea_ecb_encrypt(pt, tmp[0], &skey);
   xtea_ecb_decrypt(tmp[0], tmp[1], &skey);

   if (XMEMCMP(tmp[0], ct, 8) != 0 || XMEMCMP(tmp[1], pt, 8) != 0) { 
      return CRYPT_FAIL_TESTVECTOR;
   }

      /* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
      for (y = 0; y < 8; y++) tmp[0][y] = 0;
      for (y = 0; y < 1000; y++) xtea_ecb_encrypt(tmp[0], tmp[0], &skey);
      for (y = 0; y < 1000; y++) xtea_ecb_decrypt(tmp[0], tmp[0], &skey);
      for (y = 0; y < 8; y++) if (tmp[0][y] != 0) return CRYPT_FAIL_TESTVECTOR;

   return CRYPT_OK;
 #endif
}

/** Terminate the context 
   @param skey    The scheduled key
*/
void xtea_done(symmetric_key *skey)
{
}

/**
  Gets suitable key size
  @param keysize [in/out] The length of the recommended key (in bytes).  This function will store the suitable size back in this variable.
  @return CRYPT_OK if the input key size is acceptable.
*/
int xtea_keysize(int *keysize)
{
   LTC_ARGCHK(keysize != NULL);
   if (*keysize < 16) {
      return CRYPT_INVALID_KEYSIZE; 
   }
   *keysize = 16;
   return CRYPT_OK;
}


#endif




/* $Source: /cvs/libtom/libtomcrypt/src/ciphers/xtea.c,v $ */
/* $Revision: 1.14 $ */
/* $Date: 2007/05/12 14:20:27 $ */
