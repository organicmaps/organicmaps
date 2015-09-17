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
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int xtea_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
   ulong32 x, sum, K[4];

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
   LOAD32H(K[0], key+0);
   LOAD32H(K[1], key+4);
   LOAD32H(K[2], key+8);
   LOAD32H(K[3], key+12);

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
   ulong32 y, z;
   int r;

   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);

   LOAD32H(y, &pt[0]);
   LOAD32H(z, &pt[4]);
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
   STORE32H(y, &ct[0]);
   STORE32H(z, &ct[4]);
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
   ulong32 y, z;
   int r;

   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);

   LOAD32H(y, &ct[0]);
   LOAD32H(z, &ct[4]);
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
   STORE32H(y, &pt[0]);
   STORE32H(z, &pt[4]);
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
    static const struct {
        unsigned char key[16], pt[8], ct[8];
    } tests[] = {
       {
         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
         { 0xde, 0xe9, 0xd4, 0xd8, 0xf7, 0x13, 0x1e, 0xd9 }
       }, {
         { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02,
           0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04 },
         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
         { 0xa5, 0x97, 0xab, 0x41, 0x76, 0x01, 0x4d, 0x72 }
       }, {
         { 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04,
           0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06 },
         { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02 },
         { 0xb1, 0xfd, 0x5d, 0xa9, 0xcc, 0x6d, 0xc9, 0xdc }
       }, {
         { 0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f,
           0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87 },
         { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87 },
         { 0x70, 0x4b, 0x31, 0x34, 0x47, 0x44, 0xdf, 0xab }
       }, {
         { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
           0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
         { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 },
         { 0x49, 0x7d, 0xf3, 0xd0, 0x72, 0x61, 0x2c, 0xb5 }
       }, {
         { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
           0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
         { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 },
         { 0xe7, 0x8f, 0x2d, 0x13, 0x74, 0x43, 0x41, 0xd8 }
       }, {
         { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
           0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
         { 0x5a, 0x5b, 0x6e, 0x27, 0x89, 0x48, 0xd7, 0x7f },
         { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 }
       }, {
         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
         { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 },
         { 0xa0, 0x39, 0x05, 0x89, 0xf8, 0xb8, 0xef, 0xa5 }
       }, {
         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
         { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 },
         { 0xed, 0x23, 0x37, 0x5a, 0x82, 0x1a, 0x8c, 0x2d }
       }, {
         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
         { 0x70, 0xe1, 0x22, 0x5d, 0x6e, 0x4e, 0x76, 0x55 },
         { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 }
       }
    };
   unsigned char tmp[2][8];
   symmetric_key skey;
   int i, err, y;
   for (i = 0; i < (int)(sizeof(tests)/sizeof(tests[0])); i++) {
       zeromem(&skey, sizeof(skey));
       if ((err = xtea_setup(tests[i].key, 16, 0, &skey)) != CRYPT_OK)  {
          return err;
       }
       xtea_ecb_encrypt(tests[i].pt, tmp[0], &skey);
       xtea_ecb_decrypt(tmp[0], tmp[1], &skey);

       if (XMEMCMP(tmp[0], tests[i].ct, 8) != 0 || XMEMCMP(tmp[1], tests[i].pt, 8) != 0) {
#if 0
          printf("\n\nTest %d failed\n", i);
          if (XMEMCMP(tmp[0], tests[i].ct, 8)) {
            printf("CT: ");
            for (i = 0; i < 8; i++) {
              printf("%02x ", tmp[0][i]);
            }
            printf("\n");
          } else {
            printf("PT: ");
            for (i = 0; i < 8; i++) {
              printf("%02x ", tmp[1][i]);
            }
            printf("\n");
          }
#endif
          return CRYPT_FAIL_TESTVECTOR;
       }

      /* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
      for (y = 0; y < 8; y++) tmp[0][y] = 0;
      for (y = 0; y < 1000; y++) xtea_ecb_encrypt(tmp[0], tmp[0], &skey);
      for (y = 0; y < 1000; y++) xtea_ecb_decrypt(tmp[0], tmp[0], &skey);
      for (y = 0; y < 8; y++) if (tmp[0][y] != 0) return CRYPT_FAIL_TESTVECTOR;
   } /* for */

   return CRYPT_OK;
 #endif
}

/** Terminate the context
   @param skey    The scheduled key
*/
void xtea_done(symmetric_key *skey)
{
  LTC_UNUSED_PARAM(skey);
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




/* $Source$ */
/* $Revision$ */
/* $Date$ */
