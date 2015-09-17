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
  @file multi2.c
  Multi-2 implementation (not public domain, hence the default disable)
*/
#include "tomcrypt.h"

#ifdef LTC_MULTI2

static void pi1(ulong32 *p)
{
   p[1] ^= p[0];
}

static void pi2(ulong32 *p, ulong32 *k)
{
   ulong32 t;
   t = (p[1] + k[0]) & 0xFFFFFFFFUL;
   t = (ROL(t, 1) + t - 1)  & 0xFFFFFFFFUL;
   t = (ROL(t, 4) ^ t)  & 0xFFFFFFFFUL;
   p[0] ^= t;
}

static void pi3(ulong32 *p, ulong32 *k)
{
   ulong32 t;
   t = p[0] + k[1];
   t = (ROL(t, 2) + t + 1)  & 0xFFFFFFFFUL;
   t = (ROL(t, 8) ^ t)  & 0xFFFFFFFFUL;
   t = (t + k[2])  & 0xFFFFFFFFUL;
   t = (ROL(t, 1) - t)  & 0xFFFFFFFFUL;
   t = ROL(t, 16) ^ (p[0] | t);
   p[1] ^= t;
}

static void pi4(ulong32 *p, ulong32 *k)
{
   ulong32 t;
   t = (p[1] + k[3])  & 0xFFFFFFFFUL;
   t = (ROL(t, 2) + t + 1)  & 0xFFFFFFFFUL;
   p[0] ^= t;
}

static void setup(ulong32 *dk, ulong32 *k, ulong32 *uk)
{
   int n, t;
   ulong32 p[2];

   p[0] = dk[0]; p[1] = dk[1];

   t = 4;
   n = 0;
      pi1(p);
      pi2(p, k);
      uk[n++] = p[0];
      pi3(p, k);
      uk[n++] = p[1];
      pi4(p, k);
      uk[n++] = p[0];
      pi1(p);
      uk[n++] = p[1];
      pi2(p, k+t);
      uk[n++] = p[0];
      pi3(p, k+t);
      uk[n++] = p[1];
      pi4(p, k+t);
      uk[n++] = p[0];
      pi1(p);
      uk[n++] = p[1];
}

static void encrypt(ulong32 *p, int N, ulong32 *uk)
{
   int n, t;
   for (t = n = 0; ; ) {
      pi1(p); if (++n == N) break;
      pi2(p, uk+t); if (++n == N) break;
      pi3(p, uk+t); if (++n == N) break;
      pi4(p, uk+t); if (++n == N) break;
      t ^= 4;
   }
}

static void decrypt(ulong32 *p, int N, ulong32 *uk)
{
   int n, t;
   for (t = 4*(((N-1)>>2)&1), n = N; ;  ) {
      switch (n<=4 ? n : ((n-1)%4)+1) {
         case 4: pi4(p, uk+t); --n;
         case 3: pi3(p, uk+t); --n;
         case 2: pi2(p, uk+t); --n;
         case 1: pi1(p); --n; break;
         case 0: return;
      }
      t ^= 4;
   }
}

const struct ltc_cipher_descriptor multi2_desc = {
   "multi2",
   22,
   40, 40, 8, 128,
   &multi2_setup,
   &multi2_ecb_encrypt,
   &multi2_ecb_decrypt,
   &multi2_test,
   &multi2_done,
   &multi2_keysize,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int  multi2_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
   ulong32 sk[8], dk[2];
   int      x;

   LTC_ARGCHK(key  != NULL);
   LTC_ARGCHK(skey != NULL);

   if (keylen != 40) return CRYPT_INVALID_KEYSIZE;
   if (num_rounds == 0) num_rounds = 128;

   skey->multi2.N = num_rounds;
   for (x = 0; x < 8; x++) {
       LOAD32H(sk[x], key + x*4);
   }
   LOAD32H(dk[0], key + 32);
   LOAD32H(dk[1], key + 36);
   setup(dk, sk, skey->multi2.uk);

   zeromem(sk, sizeof(sk));
   zeromem(dk, sizeof(dk));
   return CRYPT_OK;
}

/**
  Encrypts a block of text with multi2
  @param pt The input plaintext (8 bytes)
  @param ct The output ciphertext (8 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
int multi2_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
   ulong32 p[2];
   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);
   LOAD32H(p[0], pt);
   LOAD32H(p[1], pt+4);
   encrypt(p, skey->multi2.N, skey->multi2.uk);
   STORE32H(p[0], ct);
   STORE32H(p[1], ct+4);
   return CRYPT_OK;
}

/**
  Decrypts a block of text with multi2
  @param ct The input ciphertext (8 bytes)
  @param pt The output plaintext (8 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
int multi2_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
   ulong32 p[2];
   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);
   LOAD32H(p[0], ct);
   LOAD32H(p[1], ct+4);
   decrypt(p, skey->multi2.N, skey->multi2.uk);
   STORE32H(p[0], pt);
   STORE32H(p[1], pt+4);
   return CRYPT_OK;
}

/**
  Performs a self-test of the multi2 block cipher
  @return CRYPT_OK if functional, CRYPT_NOP if self-test has been disabled
*/
int multi2_test(void)
{
   static const struct {
      unsigned char key[40];
      unsigned char pt[8], ct[8];
      int           rounds;
   } tests[] = {
{
   {
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,

      0x01, 0x23, 0x45, 0x67,
      0x89, 0xAB, 0xCD, 0xEF
   },
   {
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01,
   },
   {
      0xf8, 0x94, 0x40, 0x84,
      0x5e, 0x11, 0xcf, 0x89
   },
   128,
},
{
   {
      0x35, 0x91, 0x9d, 0x96,
      0x07, 0x02, 0xe2, 0xce,
      0x8d, 0x0b, 0x58, 0x3c,
      0xc9, 0xc8, 0x9d, 0x59,
      0xa2, 0xae, 0x96, 0x4e,
      0x87, 0x82, 0x45, 0xed,
      0x3f, 0x2e, 0x62, 0xd6,
      0x36, 0x35, 0xd0, 0x67,

      0xb1, 0x27, 0xb9, 0x06,
      0xe7, 0x56, 0x22, 0x38,
   },
   {
      0x1f, 0xb4, 0x60, 0x60,
      0xd0, 0xb3, 0x4f, 0xa5
   },
   {
      0xca, 0x84, 0xa9, 0x34,
      0x75, 0xc8, 0x60, 0xe5
   },
   216,
}
};
   unsigned char buf[8];
   symmetric_key skey;
   int err, x;

   for (x = 1; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
      if ((err = multi2_setup(tests[x].key, 40, tests[x].rounds, &skey)) != CRYPT_OK) {
         return err;
      }
      if ((err = multi2_ecb_encrypt(tests[x].pt, buf, &skey)) != CRYPT_OK) {
         return err;
      }

      if (XMEMCMP(buf, tests[x].ct, 8)) {
         return CRYPT_FAIL_TESTVECTOR;
      }

      if ((err = multi2_ecb_decrypt(buf, buf, &skey)) != CRYPT_OK) {
         return err;
      }
      if (XMEMCMP(buf, tests[x].pt, 8)) {
         return CRYPT_FAIL_TESTVECTOR;
      }
   }

   for (x = 128; x < 256; ++x) {
        unsigned char ct[8];

        if ((err = multi2_setup(tests[0].key, 40, x, &skey)) != CRYPT_OK) {
                return err;
        }
        if ((err = multi2_ecb_encrypt(tests[0].pt, ct, &skey)) != CRYPT_OK) {
                return err;
        }
        if ((err = multi2_ecb_decrypt(ct, buf, &skey)) != CRYPT_OK) {
                return err;
        }
        if (XMEMCMP(buf, tests[0].pt, 8)) {
                return CRYPT_FAIL_TESTVECTOR;
        }
   }

   return CRYPT_OK;
}

/** Terminate the context
   @param skey    The scheduled key
*/
void multi2_done(symmetric_key *skey)
{
  LTC_UNUSED_PARAM(skey);
}

/**
  Gets suitable key size
  @param keysize [in/out] The length of the recommended key (in bytes).  This function will store the suitable size back in this variable.
  @return CRYPT_OK if the input key size is acceptable.
*/
int multi2_keysize(int *keysize)
{
   LTC_ARGCHK(keysize != NULL);
   if (*keysize >= 40) {
      *keysize = 40;
   } else {
      return CRYPT_INVALID_KEYSIZE;
   }
   return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
