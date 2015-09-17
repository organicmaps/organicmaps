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
   @file noekeon.c
   Implementation of the Noekeon block cipher by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_NOEKEON

const struct ltc_cipher_descriptor noekeon_desc =
{
    "noekeon",
    16,
    16, 16, 16, 16,
    &noekeon_setup,
    &noekeon_ecb_encrypt,
    &noekeon_ecb_decrypt,
    &noekeon_test,
    &noekeon_done,
    &noekeon_keysize,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static const ulong32 RC[] = {
   0x00000080UL, 0x0000001bUL, 0x00000036UL, 0x0000006cUL,
   0x000000d8UL, 0x000000abUL, 0x0000004dUL, 0x0000009aUL,
   0x0000002fUL, 0x0000005eUL, 0x000000bcUL, 0x00000063UL,
   0x000000c6UL, 0x00000097UL, 0x00000035UL, 0x0000006aUL,
   0x000000d4UL
};

#define kTHETA(a, b, c, d)                                 \
    temp = a^c; temp = temp ^ ROLc(temp, 8) ^ RORc(temp, 8); \
    b ^= temp; d ^= temp;                                  \
    temp = b^d; temp = temp ^ ROLc(temp, 8) ^ RORc(temp, 8); \
    a ^= temp; c ^= temp;

#define THETA(k, a, b, c, d)                               \
    temp = a^c; temp = temp ^ ROLc(temp, 8) ^ RORc(temp, 8); \
    b ^= temp ^ k[1]; d ^= temp ^ k[3];                    \
    temp = b^d; temp = temp ^ ROLc(temp, 8) ^ RORc(temp, 8); \
    a ^= temp ^ k[0]; c ^= temp ^ k[2];

#define GAMMA(a, b, c, d)     \
    b ^= ~(d|c);              \
    a ^= c&b;                 \
    temp = d; d = a; a = temp;\
    c ^= a ^ b ^ d;           \
    b ^= ~(d|c);              \
    a ^= c&b;

#define PI1(a, b, c, d) \
    b = ROLc(b, 1); c = ROLc(c, 5); d = ROLc(d, 2);

#define PI2(a, b, c, d) \
    b = RORc(b, 1); c = RORc(c, 5); d = RORc(d, 2);

 /**
    Initialize the Noekeon block cipher
    @param key The symmetric key you wish to pass
    @param keylen The key length in bytes
    @param num_rounds The number of rounds desired (0 for default)
    @param skey The key in as scheduled by this function.
    @return CRYPT_OK if successful
 */
int noekeon_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
   ulong32 temp;

   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(skey != NULL);

   if (keylen != 16) {
      return CRYPT_INVALID_KEYSIZE;
   }

   if (num_rounds != 16 && num_rounds != 0) {
      return CRYPT_INVALID_ROUNDS;
   }

   LOAD32H(skey->noekeon.K[0],&key[0]);
   LOAD32H(skey->noekeon.K[1],&key[4]);
   LOAD32H(skey->noekeon.K[2],&key[8]);
   LOAD32H(skey->noekeon.K[3],&key[12]);

   LOAD32H(skey->noekeon.dK[0],&key[0]);
   LOAD32H(skey->noekeon.dK[1],&key[4]);
   LOAD32H(skey->noekeon.dK[2],&key[8]);
   LOAD32H(skey->noekeon.dK[3],&key[12]);

   kTHETA(skey->noekeon.dK[0], skey->noekeon.dK[1], skey->noekeon.dK[2], skey->noekeon.dK[3]);

   return CRYPT_OK;
}

/**
  Encrypts a block of text with Noekeon
  @param pt The input plaintext (16 bytes)
  @param ct The output ciphertext (16 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _noekeon_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#else
int noekeon_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#endif
{
   ulong32 a,b,c,d,temp;
   int r;

   LTC_ARGCHK(skey != NULL);
   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);

   LOAD32H(a,&pt[0]); LOAD32H(b,&pt[4]);
   LOAD32H(c,&pt[8]); LOAD32H(d,&pt[12]);

#define ROUND(i) \
       a ^= RC[i]; \
       THETA(skey->noekeon.K, a,b,c,d); \
       PI1(a,b,c,d); \
       GAMMA(a,b,c,d); \
       PI2(a,b,c,d);

   for (r = 0; r < 16; ++r) {
       ROUND(r);
   }

#undef ROUND

   a ^= RC[16];
   THETA(skey->noekeon.K, a, b, c, d);

   STORE32H(a,&ct[0]); STORE32H(b,&ct[4]);
   STORE32H(c,&ct[8]); STORE32H(d,&ct[12]);

   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int noekeon_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
   int err = _noekeon_ecb_encrypt(pt, ct, skey);
   burn_stack(sizeof(ulong32) * 5 + sizeof(int));
   return err;
}
#endif

/**
  Decrypts a block of text with Noekeon
  @param ct The input ciphertext (16 bytes)
  @param pt The output plaintext (16 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _noekeon_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#else
int noekeon_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#endif
{
   ulong32 a,b,c,d, temp;
   int r;

   LTC_ARGCHK(skey != NULL);
   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);

   LOAD32H(a,&ct[0]); LOAD32H(b,&ct[4]);
   LOAD32H(c,&ct[8]); LOAD32H(d,&ct[12]);


#define ROUND(i) \
       THETA(skey->noekeon.dK, a,b,c,d); \
       a ^= RC[i]; \
       PI1(a,b,c,d); \
       GAMMA(a,b,c,d); \
       PI2(a,b,c,d);

   for (r = 16; r > 0; --r) {
       ROUND(r);
   }

#undef ROUND

   THETA(skey->noekeon.dK, a,b,c,d);
   a ^= RC[0];
   STORE32H(a,&pt[0]); STORE32H(b, &pt[4]);
   STORE32H(c,&pt[8]); STORE32H(d, &pt[12]);
   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int noekeon_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
   int err = _noekeon_ecb_decrypt(ct, pt, skey);
   burn_stack(sizeof(ulong32) * 5 + sizeof(int));
   return err;
}
#endif

/**
  Performs a self-test of the Noekeon block cipher
  @return CRYPT_OK if functional, CRYPT_NOP if self-test has been disabled
*/
int noekeon_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
 static const struct {
     int keylen;
     unsigned char key[16], pt[16], ct[16];
 } tests[] = {
   {
      16,
      { 0xAA, 0x3C, 0x8C, 0x86, 0xD9, 0x8B, 0xF8, 0xBE, 0x21, 0xE0, 0x36, 0x09, 0x78, 0xFB, 0xE4, 0x90 },
      { 0xE4, 0x96, 0x6C, 0xD3, 0x13, 0xA0, 0x6C, 0xAF, 0xD0, 0x23, 0xC9, 0xFD, 0x45, 0x32, 0x23, 0x16 },
      { 0xA6, 0xEC, 0xB8, 0xA8, 0x61, 0xFD, 0x62, 0xD9, 0x13, 0x02, 0xFE, 0x9E, 0x47, 0x01, 0x3F, 0xC3 }
   },
   {
      16,
      { 0xED, 0x43, 0xD1, 0x87, 0x21, 0x7E, 0xE0, 0x97, 0x3D, 0x76, 0xC3, 0x37, 0x2E, 0x7D, 0xAE, 0xD3 },
      { 0xE3, 0x38, 0x32, 0xCC, 0xF2, 0x2F, 0x2F, 0x0A, 0x4A, 0x8B, 0x8F, 0x18, 0x12, 0x20, 0x17, 0xD3 },
      { 0x94, 0xA5, 0xDF, 0xF5, 0xAE, 0x1C, 0xBB, 0x22, 0xAD, 0xEB, 0xA7, 0x0D, 0xB7, 0x82, 0x90, 0xA0 }
   },
   {
      16,
      { 0x6F, 0xDC, 0x23, 0x38, 0xF2, 0x10, 0xFB, 0xD3, 0xC1, 0x8C, 0x02, 0xF6, 0xB4, 0x6A, 0xD5, 0xA8 },
      { 0xDB, 0x29, 0xED, 0xB5, 0x5F, 0xB3, 0x60, 0x3A, 0x92, 0xA8, 0xEB, 0x9C, 0x6D, 0x9D, 0x3E, 0x8F },
      { 0x78, 0xF3, 0x6F, 0xF8, 0x9E, 0xBB, 0x8C, 0x6A, 0xE8, 0x10, 0xF7, 0x00, 0x22, 0x15, 0x30, 0x3D }
   },
   {
      16,
      { 0x2C, 0x0C, 0x02, 0xEF, 0x6B, 0xC4, 0xF2, 0x0B, 0x2E, 0xB9, 0xE0, 0xBF, 0xD9, 0x36, 0xC2, 0x4E },
      { 0x84, 0xE2, 0xFE, 0x64, 0xB1, 0xB9, 0xFE, 0x76, 0xA8, 0x3F, 0x45, 0xC7, 0x40, 0x7A, 0xAF, 0xEE },
      { 0x2A, 0x08, 0xD6, 0xA2, 0x1C, 0x63, 0x08, 0xB0, 0xF8, 0xBC, 0xB3, 0xA1, 0x66, 0xF7, 0xAE, 0xCF }
   },
   {
      16,
      { 0x6F, 0x30, 0xF8, 0x9F, 0xDA, 0x6E, 0xA0, 0x91, 0x04, 0x0F, 0x6C, 0x8B, 0x7D, 0xF7, 0x2A, 0x4B },
      { 0x65, 0xB6, 0xA6, 0xD0, 0x42, 0x14, 0x08, 0x60, 0x34, 0x8D, 0x37, 0x2F, 0x01, 0xF0, 0x46, 0xBE },
      { 0x66, 0xAC, 0x0B, 0x62, 0x1D, 0x68, 0x11, 0xF5, 0x27, 0xB1, 0x13, 0x5D, 0xF3, 0x2A, 0xE9, 0x18 }
   },
   {
      16,
      { 0xCA, 0xA4, 0x16, 0xB7, 0x1C, 0x92, 0x2E, 0xAD, 0xEB, 0xA7, 0xDB, 0x69, 0x92, 0xCB, 0x35, 0xEF },
      { 0x81, 0x6F, 0x8E, 0x4D, 0x96, 0xC6, 0xB3, 0x67, 0x83, 0xF5, 0x63, 0xC7, 0x20, 0x6D, 0x40, 0x23 },
      { 0x44, 0xF7, 0x63, 0x62, 0xF0, 0x43, 0xBB, 0x67, 0x4A, 0x75, 0x12, 0x42, 0x46, 0x29, 0x28, 0x19 }
   },
   {
      16,
      { 0x6B, 0xCF, 0x22, 0x2F, 0xE0, 0x1B, 0xB0, 0xAA, 0xD8, 0x3C, 0x91, 0x99, 0x18, 0xB2, 0x28, 0xE8 },
      { 0x7C, 0x37, 0xC7, 0xD0, 0xAC, 0x92, 0x29, 0xF1, 0x60, 0x82, 0x93, 0x89, 0xAA, 0x61, 0xAA, 0xA9 },
      { 0xE5, 0x89, 0x1B, 0xB3, 0xFE, 0x8B, 0x0C, 0xA1, 0xA6, 0xC7, 0xBE, 0x12, 0x73, 0x0F, 0xC1, 0x19 }
   },
   {
      16,
      { 0xE6, 0xD0, 0xF1, 0x03, 0x2E, 0xDE, 0x70, 0x8D, 0xD8, 0x9E, 0x36, 0x5C, 0x05, 0x52, 0xE7, 0x0D },
      { 0xE2, 0x42, 0xE7, 0x92, 0x0E, 0xF7, 0x82, 0xA2, 0xB8, 0x21, 0x8D, 0x26, 0xBA, 0x2D, 0xE6, 0x32 },
      { 0x1E, 0xDD, 0x75, 0x22, 0xB9, 0x36, 0x8A, 0x0F, 0x32, 0xFD, 0xD4, 0x48, 0x65, 0x12, 0x5A, 0x2F }
   }
 };
 symmetric_key key;
 unsigned char tmp[2][16];
 int err, i, y;

 for (i = 0; i < (int)(sizeof(tests)/sizeof(tests[0])); i++) {
    zeromem(&key, sizeof(key));
    if ((err = noekeon_setup(tests[i].key, tests[i].keylen, 0, &key)) != CRYPT_OK) {
       return err;
    }

    noekeon_ecb_encrypt(tests[i].pt, tmp[0], &key);
    noekeon_ecb_decrypt(tmp[0], tmp[1], &key);
    if (XMEMCMP(tmp[0], tests[i].ct, 16) || XMEMCMP(tmp[1], tests[i].pt, 16)) {
#if 0
       printf("\n\nTest %d failed\n", i);
       if (XMEMCMP(tmp[0], tests[i].ct, 16)) {
          printf("CT: ");
          for (i = 0; i < 16; i++) {
             printf("%02x ", tmp[0][i]);
          }
          printf("\n");
       } else {
          printf("PT: ");
          for (i = 0; i < 16; i++) {
             printf("%02x ", tmp[1][i]);
          }
          printf("\n");
       }
#endif
        return CRYPT_FAIL_TESTVECTOR;
    }

      /* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
      for (y = 0; y < 16; y++) tmp[0][y] = 0;
      for (y = 0; y < 1000; y++) noekeon_ecb_encrypt(tmp[0], tmp[0], &key);
      for (y = 0; y < 1000; y++) noekeon_ecb_decrypt(tmp[0], tmp[0], &key);
      for (y = 0; y < 16; y++) if (tmp[0][y] != 0) return CRYPT_FAIL_TESTVECTOR;
 }
 return CRYPT_OK;
 #endif
}

/** Terminate the context
   @param skey    The scheduled key
*/
void noekeon_done(symmetric_key *skey)
{
  LTC_UNUSED_PARAM(skey);
}

/**
  Gets suitable key size
  @param keysize [in/out] The length of the recommended key (in bytes).  This function will store the suitable size back in this variable.
  @return CRYPT_OK if the input key size is acceptable.
*/
int noekeon_keysize(int *keysize)
{
   LTC_ARGCHK(keysize != NULL);
   if (*keysize < 16) {
      return CRYPT_INVALID_KEYSIZE;
   } else {
      *keysize = 16;
      return CRYPT_OK;
   }
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */
