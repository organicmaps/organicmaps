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
  @file skipjack.c
  Skipjack Implementation by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_SKIPJACK

const struct ltc_cipher_descriptor skipjack_desc =
{
    "skipjack",
    17,
    10, 10, 8, 32,
    &skipjack_setup,
    &skipjack_ecb_encrypt,
    &skipjack_ecb_decrypt,
    &skipjack_test,
    &skipjack_done,
    &skipjack_keysize,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static const unsigned char sbox[256] = {
   0xa3,0xd7,0x09,0x83,0xf8,0x48,0xf6,0xf4,0xb3,0x21,0x15,0x78,0x99,0xb1,0xaf,0xf9,
   0xe7,0x2d,0x4d,0x8a,0xce,0x4c,0xca,0x2e,0x52,0x95,0xd9,0x1e,0x4e,0x38,0x44,0x28,
   0x0a,0xdf,0x02,0xa0,0x17,0xf1,0x60,0x68,0x12,0xb7,0x7a,0xc3,0xe9,0xfa,0x3d,0x53,
   0x96,0x84,0x6b,0xba,0xf2,0x63,0x9a,0x19,0x7c,0xae,0xe5,0xf5,0xf7,0x16,0x6a,0xa2,
   0x39,0xb6,0x7b,0x0f,0xc1,0x93,0x81,0x1b,0xee,0xb4,0x1a,0xea,0xd0,0x91,0x2f,0xb8,
   0x55,0xb9,0xda,0x85,0x3f,0x41,0xbf,0xe0,0x5a,0x58,0x80,0x5f,0x66,0x0b,0xd8,0x90,
   0x35,0xd5,0xc0,0xa7,0x33,0x06,0x65,0x69,0x45,0x00,0x94,0x56,0x6d,0x98,0x9b,0x76,
   0x97,0xfc,0xb2,0xc2,0xb0,0xfe,0xdb,0x20,0xe1,0xeb,0xd6,0xe4,0xdd,0x47,0x4a,0x1d,
   0x42,0xed,0x9e,0x6e,0x49,0x3c,0xcd,0x43,0x27,0xd2,0x07,0xd4,0xde,0xc7,0x67,0x18,
   0x89,0xcb,0x30,0x1f,0x8d,0xc6,0x8f,0xaa,0xc8,0x74,0xdc,0xc9,0x5d,0x5c,0x31,0xa4,
   0x70,0x88,0x61,0x2c,0x9f,0x0d,0x2b,0x87,0x50,0x82,0x54,0x64,0x26,0x7d,0x03,0x40,
   0x34,0x4b,0x1c,0x73,0xd1,0xc4,0xfd,0x3b,0xcc,0xfb,0x7f,0xab,0xe6,0x3e,0x5b,0xa5,
   0xad,0x04,0x23,0x9c,0x14,0x51,0x22,0xf0,0x29,0x79,0x71,0x7e,0xff,0x8c,0x0e,0xe2,
   0x0c,0xef,0xbc,0x72,0x75,0x6f,0x37,0xa1,0xec,0xd3,0x8e,0x62,0x8b,0x86,0x10,0xe8,
   0x08,0x77,0x11,0xbe,0x92,0x4f,0x24,0xc5,0x32,0x36,0x9d,0xcf,0xf3,0xa6,0xbb,0xac,
   0x5e,0x6c,0xa9,0x13,0x57,0x25,0xb5,0xe3,0xbd,0xa8,0x3a,0x01,0x05,0x59,0x2a,0x46
};

/* simple x + 1 (mod 10) in one step. */
static const int keystep[] =  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

/* simple x - 1 (mod 10) in one step */
static const int ikeystep[] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

 /**
    Initialize the Skipjack block cipher
    @param key The symmetric key you wish to pass
    @param keylen The key length in bytes
    @param num_rounds The number of rounds desired (0 for default)
    @param skey The key in as scheduled by this function.
    @return CRYPT_OK if successful
 */
int skipjack_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
   int x;

   LTC_ARGCHK(key  != NULL);
   LTC_ARGCHK(skey != NULL);

   if (keylen != 10) {
      return CRYPT_INVALID_KEYSIZE;
   }

   if (num_rounds != 32 && num_rounds != 0) { 
      return CRYPT_INVALID_ROUNDS;
   }

   /* make sure the key is in range for platforms where CHAR_BIT != 8 */
   for (x = 0; x < 10; x++) {
       skey->skipjack.key[x] = key[x] & 255;
   }

   return CRYPT_OK;
}

#define RULE_A \
   tmp = g_func(w1, &kp, skey->skipjack.key);      \
   w1  = tmp ^ w4 ^ x;                            \
   w4  = w3; w3 = w2;                             \
   w2  = tmp;

#define RULE_B \
   tmp  = g_func(w1, &kp, skey->skipjack.key);     \
   tmp1 = w4; w4  = w3;                           \
   w3   = w1 ^ w2 ^ x;                            \
   w1   = tmp1; w2 = tmp;

#define RULE_A1 \
   tmp = w1 ^ w2 ^ x;                             \
   w1  = ig_func(w2, &kp, skey->skipjack.key);     \
   w2  = w3; w3 = w4; w4 = tmp;

#define RULE_B1 \
   tmp = ig_func(w2, &kp, skey->skipjack.key);     \
   w2  = tmp ^ w3 ^ x;                            \
   w3  = w4; w4 = w1; w1 = tmp;

static unsigned g_func(unsigned w, int *kp, unsigned char *key)
{
   unsigned char g1,g2;

   g1 = (w >> 8) & 255; g2 = w & 255;
   g1 ^= sbox[g2^key[*kp]]; *kp = keystep[*kp];
   g2 ^= sbox[g1^key[*kp]]; *kp = keystep[*kp];
   g1 ^= sbox[g2^key[*kp]]; *kp = keystep[*kp];
   g2 ^= sbox[g1^key[*kp]]; *kp = keystep[*kp];
   return ((unsigned)g1<<8)|(unsigned)g2;
}

static unsigned ig_func(unsigned w, int *kp, unsigned char *key)
{
   unsigned char g1,g2;

   g1 = (w >> 8) & 255; g2 = w & 255;
   *kp = ikeystep[*kp]; g2 ^= sbox[g1^key[*kp]];
   *kp = ikeystep[*kp]; g1 ^= sbox[g2^key[*kp]];
   *kp = ikeystep[*kp]; g2 ^= sbox[g1^key[*kp]];
   *kp = ikeystep[*kp]; g1 ^= sbox[g2^key[*kp]];
   return ((unsigned)g1<<8)|(unsigned)g2;
}

/**
  Encrypts a block of text with Skipjack
  @param pt The input plaintext (8 bytes)
  @param ct The output ciphertext (8 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _skipjack_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#else
int skipjack_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#endif
{
   unsigned w1,w2,w3,w4,tmp,tmp1;
   int x, kp;

   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);

   /* load block */
   w1 = ((unsigned)pt[0]<<8)|pt[1];
   w2 = ((unsigned)pt[2]<<8)|pt[3];
   w3 = ((unsigned)pt[4]<<8)|pt[5];
   w4 = ((unsigned)pt[6]<<8)|pt[7];

   /* 8 rounds of RULE A */
   for (x = 1, kp = 0; x < 9; x++) {
       RULE_A;
   }

   /* 8 rounds of RULE B */
   for (; x < 17; x++) {
       RULE_B;
   }

   /* 8 rounds of RULE A */
   for (; x < 25; x++) {
       RULE_A;
   }

   /* 8 rounds of RULE B */
   for (; x < 33; x++) {
       RULE_B;
   }

   /* store block */
   ct[0] = (w1>>8)&255; ct[1] = w1&255;
   ct[2] = (w2>>8)&255; ct[3] = w2&255;
   ct[4] = (w3>>8)&255; ct[5] = w3&255;
   ct[6] = (w4>>8)&255; ct[7] = w4&255;

   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int skipjack_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
   int err = _skipjack_ecb_encrypt(pt, ct, skey);
   burn_stack(sizeof(unsigned) * 8 + sizeof(int) * 2);
   return err;
}
#endif

/**
  Decrypts a block of text with Skipjack
  @param ct The input ciphertext (8 bytes)
  @param pt The output plaintext (8 bytes)
  @param skey The key as scheduled 
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _skipjack_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#else
int skipjack_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#endif
{
   unsigned w1,w2,w3,w4,tmp;
   int x, kp;

   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);

   /* load block */
   w1 = ((unsigned)ct[0]<<8)|ct[1];
   w2 = ((unsigned)ct[2]<<8)|ct[3];
   w3 = ((unsigned)ct[4]<<8)|ct[5];
   w4 = ((unsigned)ct[6]<<8)|ct[7];

   /* 8 rounds of RULE B^-1 

      Note the value "kp = 8" comes from "kp = (32 * 4) mod 10" where 32*4 is 128 which mod 10 is 8
    */
   for (x = 32, kp = 8; x > 24; x--) {
       RULE_B1;
   }

   /* 8 rounds of RULE A^-1 */
   for (; x > 16; x--) {
       RULE_A1;
   }


   /* 8 rounds of RULE B^-1 */
   for (; x > 8; x--) {
       RULE_B1;
   }

   /* 8 rounds of RULE A^-1 */
   for (; x > 0; x--) {
       RULE_A1;
   }

   /* store block */
   pt[0] = (w1>>8)&255; pt[1] = w1&255;
   pt[2] = (w2>>8)&255; pt[3] = w2&255;
   pt[4] = (w3>>8)&255; pt[5] = w3&255;
   pt[6] = (w4>>8)&255; pt[7] = w4&255;

   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int skipjack_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
   int err = _skipjack_ecb_decrypt(ct, pt, skey);
   burn_stack(sizeof(unsigned) * 7 + sizeof(int) * 2);
   return err;
}
#endif

/**
  Performs a self-test of the Skipjack block cipher
  @return CRYPT_OK if functional, CRYPT_NOP if self-test has been disabled
*/
int skipjack_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else    
   static const struct {
       unsigned char key[10], pt[8], ct[8];
   } tests[] = {
   {
       { 0x00, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 },
       { 0x33, 0x22, 0x11, 0x00, 0xdd, 0xcc, 0xbb, 0xaa },
       { 0x25, 0x87, 0xca, 0xe2, 0x7a, 0x12, 0xd3, 0x00 }
   }
   };
   unsigned char buf[2][8];
   int x, y, err;
   symmetric_key key;

   for (x = 0; x < (int)(sizeof(tests) / sizeof(tests[0])); x++) {
      /* setup key */
      if ((err = skipjack_setup(tests[x].key, 10, 0, &key)) != CRYPT_OK) {
         return err;
      }

      /* encrypt and decrypt */
      skipjack_ecb_encrypt(tests[x].pt, buf[0], &key);
      skipjack_ecb_decrypt(buf[0], buf[1], &key);

      /* compare */
      if (XMEMCMP(buf[0], tests[x].ct, 8) != 0 || XMEMCMP(buf[1], tests[x].pt, 8) != 0) {
         return CRYPT_FAIL_TESTVECTOR;
      }

      /* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
      for (y = 0; y < 8; y++) buf[0][y] = 0;
      for (y = 0; y < 1000; y++) skipjack_ecb_encrypt(buf[0], buf[0], &key);
      for (y = 0; y < 1000; y++) skipjack_ecb_decrypt(buf[0], buf[0], &key);
      for (y = 0; y < 8; y++) if (buf[0][y] != 0) return CRYPT_FAIL_TESTVECTOR;
   }

   return CRYPT_OK;
  #endif
}

/** Terminate the context 
   @param skey    The scheduled key
*/
void skipjack_done(symmetric_key *skey)
{
}

/**
  Gets suitable key size
  @param keysize [in/out] The length of the recommended key (in bytes).  This function will store the suitable size back in this variable.
  @return CRYPT_OK if the input key size is acceptable.
*/
int skipjack_keysize(int *keysize)
{
   LTC_ARGCHK(keysize != NULL);
   if (*keysize < 10) {
      return CRYPT_INVALID_KEYSIZE;
   } else if (*keysize > 10) {
      *keysize = 10;
   }
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/ciphers/skipjack.c,v $ */
/* $Revision: 1.14 $ */
/* $Date: 2007/05/12 14:20:27 $ */
