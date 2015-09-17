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
   @file rc5.c
   LTC_RC5 code by Tom St Denis
*/

#include "tomcrypt.h"

#ifdef LTC_RC5

const struct ltc_cipher_descriptor rc5_desc =
{
    "rc5",
    2,
    8, 128, 8, 12,
    &rc5_setup,
    &rc5_ecb_encrypt,
    &rc5_ecb_decrypt,
    &rc5_test,
    &rc5_done,
    &rc5_keysize,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static const ulong32 stab[50] = {
0xb7e15163UL, 0x5618cb1cUL, 0xf45044d5UL, 0x9287be8eUL, 0x30bf3847UL, 0xcef6b200UL, 0x6d2e2bb9UL, 0x0b65a572UL,
0xa99d1f2bUL, 0x47d498e4UL, 0xe60c129dUL, 0x84438c56UL, 0x227b060fUL, 0xc0b27fc8UL, 0x5ee9f981UL, 0xfd21733aUL,
0x9b58ecf3UL, 0x399066acUL, 0xd7c7e065UL, 0x75ff5a1eUL, 0x1436d3d7UL, 0xb26e4d90UL, 0x50a5c749UL, 0xeedd4102UL,
0x8d14babbUL, 0x2b4c3474UL, 0xc983ae2dUL, 0x67bb27e6UL, 0x05f2a19fUL, 0xa42a1b58UL, 0x42619511UL, 0xe0990ecaUL,
0x7ed08883UL, 0x1d08023cUL, 0xbb3f7bf5UL, 0x5976f5aeUL, 0xf7ae6f67UL, 0x95e5e920UL, 0x341d62d9UL, 0xd254dc92UL,
0x708c564bUL, 0x0ec3d004UL, 0xacfb49bdUL, 0x4b32c376UL, 0xe96a3d2fUL, 0x87a1b6e8UL, 0x25d930a1UL, 0xc410aa5aUL,
0x62482413UL, 0x007f9dccUL
};

 /**
    Initialize the LTC_RC5 block cipher
    @param key The symmetric key you wish to pass
    @param keylen The key length in bytes
    @param num_rounds The number of rounds desired (0 for default)
    @param skey The key in as scheduled by this function.
    @return CRYPT_OK if successful
 */
#ifdef LTC_CLEAN_STACK
static int _rc5_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
#else
int rc5_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
#endif
{
    ulong32 L[64], *S, A, B, i, j, v, s, t, l;

    LTC_ARGCHK(skey != NULL);
    LTC_ARGCHK(key  != NULL);

    /* test parameters */
    if (num_rounds == 0) {
       num_rounds = rc5_desc.default_rounds;
    }

    if (num_rounds < 12 || num_rounds > 24) {
       return CRYPT_INVALID_ROUNDS;
    }

    /* key must be between 64 and 1024 bits */
    if (keylen < 8 || keylen > 128) {
       return CRYPT_INVALID_KEYSIZE;
    }

    skey->rc5.rounds = num_rounds;
    S = skey->rc5.K;

    /* copy the key into the L array */
    for (A = i = j = 0; i < (ulong32)keylen; ) {
        A = (A << 8) | ((ulong32)(key[i++] & 255));
        if ((i & 3) == 0) {
           L[j++] = BSWAP(A);
           A = 0;
        }
    }

    if ((keylen & 3) != 0) {
       A <<= (ulong32)((8 * (4 - (keylen&3))));
       L[j++] = BSWAP(A);
    }

    /* setup the S array */
    t = (ulong32)(2 * (num_rounds + 1));
    XMEMCPY(S, stab, t * sizeof(*S));

    /* mix buffer */
    s = 3 * MAX(t, j);
    l = j;
    for (A = B = i = j = v = 0; v < s; v++) {
        A = S[i] = ROLc(S[i] + A + B, 3);
        B = L[j] = ROL(L[j] + A + B, (A+B));
        if (++i == t) { i = 0; }
        if (++j == l) { j = 0; }
    }
    return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int rc5_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
   int x;
   x = _rc5_setup(key, keylen, num_rounds, skey);
   burn_stack(sizeof(ulong32) * 122 + sizeof(int));
   return x;
}
#endif

/**
  Encrypts a block of text with LTC_RC5
  @param pt The input plaintext (8 bytes)
  @param ct The output ciphertext (8 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _rc5_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#else
int rc5_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#endif
{
   ulong32 A, B, *K;
   int r;
   LTC_ARGCHK(skey != NULL);
   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);

   LOAD32L(A, &pt[0]);
   LOAD32L(B, &pt[4]);
   A += skey->rc5.K[0];
   B += skey->rc5.K[1];
   K  = skey->rc5.K + 2;

   if ((skey->rc5.rounds & 1) == 0) {
      for (r = 0; r < skey->rc5.rounds; r += 2) {
          A = ROL(A ^ B, B) + K[0];
          B = ROL(B ^ A, A) + K[1];
          A = ROL(A ^ B, B) + K[2];
          B = ROL(B ^ A, A) + K[3];
          K += 4;
      }
   } else {
      for (r = 0; r < skey->rc5.rounds; r++) {
          A = ROL(A ^ B, B) + K[0];
          B = ROL(B ^ A, A) + K[1];
          K += 2;
      }
   }
   STORE32L(A, &ct[0]);
   STORE32L(B, &ct[4]);

   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int rc5_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
   int err = _rc5_ecb_encrypt(pt, ct, skey);
   burn_stack(sizeof(ulong32) * 2 + sizeof(int));
   return err;
}
#endif

/**
  Decrypts a block of text with LTC_RC5
  @param ct The input ciphertext (8 bytes)
  @param pt The output plaintext (8 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _rc5_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#else
int rc5_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#endif
{
   ulong32 A, B, *K;
   int r;
   LTC_ARGCHK(skey != NULL);
   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);

   LOAD32L(A, &ct[0]);
   LOAD32L(B, &ct[4]);
   K = skey->rc5.K + (skey->rc5.rounds << 1);

   if ((skey->rc5.rounds & 1) == 0) {
       K -= 2;
       for (r = skey->rc5.rounds - 1; r >= 0; r -= 2) {
          B = ROR(B - K[3], A) ^ A;
          A = ROR(A - K[2], B) ^ B;
          B = ROR(B - K[1], A) ^ A;
          A = ROR(A - K[0], B) ^ B;
          K -= 4;
        }
   } else {
      for (r = skey->rc5.rounds - 1; r >= 0; r--) {
          B = ROR(B - K[1], A) ^ A;
          A = ROR(A - K[0], B) ^ B;
          K -= 2;
      }
   }
   A -= skey->rc5.K[0];
   B -= skey->rc5.K[1];
   STORE32L(A, &pt[0]);
   STORE32L(B, &pt[4]);

   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int rc5_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
   int err = _rc5_ecb_decrypt(ct, pt, skey);
   burn_stack(sizeof(ulong32) * 2 + sizeof(int));
   return err;
}
#endif

/**
  Performs a self-test of the LTC_RC5 block cipher
  @return CRYPT_OK if functional, CRYPT_NOP if self-test has been disabled
*/
int rc5_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
   static const struct {
       unsigned char key[16], pt[8], ct[8];
   } tests[] = {
   {
       { 0x91, 0x5f, 0x46, 0x19, 0xbe, 0x41, 0xb2, 0x51,
         0x63, 0x55, 0xa5, 0x01, 0x10, 0xa9, 0xce, 0x91 },
       { 0x21, 0xa5, 0xdb, 0xee, 0x15, 0x4b, 0x8f, 0x6d },
       { 0xf7, 0xc0, 0x13, 0xac, 0x5b, 0x2b, 0x89, 0x52 }
   },
   {
       { 0x78, 0x33, 0x48, 0xe7, 0x5a, 0xeb, 0x0f, 0x2f,
         0xd7, 0xb1, 0x69, 0xbb, 0x8d, 0xc1, 0x67, 0x87 },
       { 0xF7, 0xC0, 0x13, 0xAC, 0x5B, 0x2B, 0x89, 0x52 },
       { 0x2F, 0x42, 0xB3, 0xB7, 0x03, 0x69, 0xFC, 0x92 }
   },
   {
       { 0xDC, 0x49, 0xdb, 0x13, 0x75, 0xa5, 0x58, 0x4f,
         0x64, 0x85, 0xb4, 0x13, 0xb5, 0xf1, 0x2b, 0xaf },
       { 0x2F, 0x42, 0xB3, 0xB7, 0x03, 0x69, 0xFC, 0x92 },
       { 0x65, 0xc1, 0x78, 0xb2, 0x84, 0xd1, 0x97, 0xcc }
   }
   };
   unsigned char tmp[2][8];
   int x, y, err;
   symmetric_key key;

   for (x = 0; x < (int)(sizeof(tests) / sizeof(tests[0])); x++) {
      /* setup key */
      if ((err = rc5_setup(tests[x].key, 16, 12, &key)) != CRYPT_OK) {
         return err;
      }

      /* encrypt and decrypt */
      rc5_ecb_encrypt(tests[x].pt, tmp[0], &key);
      rc5_ecb_decrypt(tmp[0], tmp[1], &key);

      /* compare */
      if (XMEMCMP(tmp[0], tests[x].ct, 8) != 0 || XMEMCMP(tmp[1], tests[x].pt, 8) != 0) {
         return CRYPT_FAIL_TESTVECTOR;
      }

      /* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
      for (y = 0; y < 8; y++) tmp[0][y] = 0;
      for (y = 0; y < 1000; y++) rc5_ecb_encrypt(tmp[0], tmp[0], &key);
      for (y = 0; y < 1000; y++) rc5_ecb_decrypt(tmp[0], tmp[0], &key);
      for (y = 0; y < 8; y++) if (tmp[0][y] != 0) return CRYPT_FAIL_TESTVECTOR;
   }
   return CRYPT_OK;
  #endif
}

/** Terminate the context
   @param skey    The scheduled key
*/
void rc5_done(symmetric_key *skey)
{
  LTC_UNUSED_PARAM(skey);
}

/**
  Gets suitable key size
  @param keysize [in/out] The length of the recommended key (in bytes).  This function will store the suitable size back in this variable.
  @return CRYPT_OK if the input key size is acceptable.
*/
int rc5_keysize(int *keysize)
{
   LTC_ARGCHK(keysize != NULL);
   if (*keysize < 8) {
      return CRYPT_INVALID_KEYSIZE;
   } else if (*keysize > 128) {
      *keysize = 128;
   }
   return CRYPT_OK;
}

#endif




/* $Source$ */
/* $Revision$ */
/* $Date$ */
