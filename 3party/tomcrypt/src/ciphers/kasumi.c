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
  @file kasumi.c
  Implementation of the 3GPP Kasumi block cipher
  Derived from the 3GPP standard source code
*/

#include "tomcrypt.h"

#ifdef LTC_KASUMI

typedef unsigned u16;

#define ROL16(x, y) ((((x)<<(y)) | ((x)>>(16-(y)))) & 0xFFFF)

const struct ltc_cipher_descriptor kasumi_desc = {
   "kasumi",
   21,
   16, 16, 8, 8,
   &kasumi_setup,
   &kasumi_ecb_encrypt,
   &kasumi_ecb_decrypt,
   &kasumi_test,
   &kasumi_done,
   &kasumi_keysize,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static u16 FI( u16 in, u16 subkey )
{
   u16 nine, seven;
   static const u16 S7[128] = {
      54, 50, 62, 56, 22, 34, 94, 96, 38, 6, 63, 93, 2, 18,123, 33,
      55,113, 39,114, 21, 67, 65, 12, 47, 73, 46, 27, 25,111,124, 81,
      53, 9,121, 79, 52, 60, 58, 48,101,127, 40,120,104, 70, 71, 43,
      20,122, 72, 61, 23,109, 13,100, 77, 1, 16, 7, 82, 10,105, 98,
      117,116, 76, 11, 89,106, 0,125,118, 99, 86, 69, 30, 57,126, 87,
      112, 51, 17, 5, 95, 14, 90, 84, 91, 8, 35,103, 32, 97, 28, 66,
      102, 31, 26, 45, 75, 4, 85, 92, 37, 74, 80, 49, 68, 29,115, 44,
      64,107,108, 24,110, 83, 36, 78, 42, 19, 15, 41, 88,119, 59, 3 };
  static const u16 S9[512] = {
      167,239,161,379,391,334, 9,338, 38,226, 48,358,452,385, 90,397,
      183,253,147,331,415,340, 51,362,306,500,262, 82,216,159,356,177,
      175,241,489, 37,206, 17, 0,333, 44,254,378, 58,143,220, 81,400,
       95, 3,315,245, 54,235,218,405,472,264,172,494,371,290,399, 76,
      165,197,395,121,257,480,423,212,240, 28,462,176,406,507,288,223,
      501,407,249,265, 89,186,221,428,164, 74,440,196,458,421,350,163,
      232,158,134,354, 13,250,491,142,191, 69,193,425,152,227,366,135,
      344,300,276,242,437,320,113,278, 11,243, 87,317, 36, 93,496, 27,
      487,446,482, 41, 68,156,457,131,326,403,339, 20, 39,115,442,124,
      475,384,508, 53,112,170,479,151,126,169, 73,268,279,321,168,364,
      363,292, 46,499,393,327,324, 24,456,267,157,460,488,426,309,229,
      439,506,208,271,349,401,434,236, 16,209,359, 52, 56,120,199,277,
      465,416,252,287,246, 6, 83,305,420,345,153,502, 65, 61,244,282,
      173,222,418, 67,386,368,261,101,476,291,195,430, 49, 79,166,330,
      280,383,373,128,382,408,155,495,367,388,274,107,459,417, 62,454,
      132,225,203,316,234, 14,301, 91,503,286,424,211,347,307,140,374,
       35,103,125,427, 19,214,453,146,498,314,444,230,256,329,198,285,
       50,116, 78,410, 10,205,510,171,231, 45,139,467, 29, 86,505, 32,
       72, 26,342,150,313,490,431,238,411,325,149,473, 40,119,174,355,
      185,233,389, 71,448,273,372, 55,110,178,322, 12,469,392,369,190,
        1,109,375,137,181, 88, 75,308,260,484, 98,272,370,275,412,111,
      336,318, 4,504,492,259,304, 77,337,435, 21,357,303,332,483, 18,
       47, 85, 25,497,474,289,100,269,296,478,270,106, 31,104,433, 84,
      414,486,394, 96, 99,154,511,148,413,361,409,255,162,215,302,201,
      266,351,343,144,441,365,108,298,251, 34,182,509,138,210,335,133,
      311,352,328,141,396,346,123,319,450,281,429,228,443,481, 92,404,
      485,422,248,297, 23,213,130,466, 22,217,283, 70,294,360,419,127,
      312,377, 7,468,194, 2,117,295,463,258,224,447,247,187, 80,398,
      284,353,105,390,299,471,470,184, 57,200,348, 63,204,188, 33,451,
       97, 30,310,219, 94,160,129,493, 64,179,263,102,189,207,114,402,
      438,477,387,122,192, 42,381, 5,145,118,180,449,293,323,136,380,
       43, 66, 60,455,341,445,202,432, 8,237, 15,376,436,464, 59,461};

  /* The sixteen bit input is split into two unequal halves, *
   * nine bits and seven bits - as is the subkey            */

  nine  = (u16)(in>>7)&0x1FF;
  seven = (u16)(in&0x7F);

  /* Now run the various operations */
  nine   = (u16)(S9[nine] ^ seven);
  seven  = (u16)(S7[seven] ^ (nine & 0x7F));
  seven ^= (subkey>>9);
  nine  ^= (subkey&0x1FF);
  nine   = (u16)(S9[nine] ^ seven);
  seven  = (u16)(S7[seven] ^ (nine & 0x7F));
  return (u16)(seven<<9) + nine;
}

static ulong32 FO( ulong32 in, int round_no, symmetric_key *key)
{
   u16 left, right;

  /* Split the input into two 16-bit words */
  left = (u16)(in>>16);
  right = (u16) in&0xFFFF;

  /* Now apply the same basic transformation three times */
  left ^= key->kasumi.KOi1[round_no];
  left = FI( left, key->kasumi.KIi1[round_no] );
  left ^= right;

  right ^= key->kasumi.KOi2[round_no];
  right = FI( right, key->kasumi.KIi2[round_no] );
  right ^= left;

  left ^= key->kasumi.KOi3[round_no];
  left = FI( left, key->kasumi.KIi3[round_no] );
  left ^= right;

  return (((ulong32)right)<<16)+left;
}

static ulong32 FL( ulong32 in, int round_no, symmetric_key *key )
{
    u16 l, r, a, b;
    /* split out the left and right halves */
    l = (u16)(in>>16);
    r = (u16)(in)&0xFFFF;
    /* do the FL() operations           */
    a = (u16) (l & key->kasumi.KLi1[round_no]);
    r ^= ROL16(a,1);
    b = (u16)(r | key->kasumi.KLi2[round_no]);
    l ^= ROL16(b,1);
    /* put the two halves back together */

    return (((ulong32)l)<<16) + r;
}

int kasumi_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
    ulong32 left, right, temp;
    int n;

    LTC_ARGCHK(pt   != NULL);
    LTC_ARGCHK(ct   != NULL);
    LTC_ARGCHK(skey != NULL);

    LOAD32H(left, pt);
    LOAD32H(right, pt+4);

    for (n = 0; n <= 7; ) {     
        temp = FL(left,  n,   skey);
        temp = FO(temp,  n++, skey);
        right ^= temp;
        temp = FO(right, n,   skey);
        temp = FL(temp,  n++, skey);
        left ^= temp;
    }

    STORE32H(left, ct);
    STORE32H(right, ct+4);

    return CRYPT_OK;
}

int kasumi_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
    ulong32 left, right, temp;
    int n;

    LTC_ARGCHK(pt   != NULL);
    LTC_ARGCHK(ct   != NULL);
    LTC_ARGCHK(skey != NULL);

    LOAD32H(left, ct);
    LOAD32H(right, ct+4);

    for (n = 7; n >= 0; ) {
        temp = FO(right, n,   skey);
        temp = FL(temp,  n--, skey);
        left ^= temp;
        temp = FL(left,  n,   skey);
        temp = FO(temp,  n--, skey);
        right ^= temp;
    }

    STORE32H(left, pt);
    STORE32H(right, pt+4);

    return CRYPT_OK;
}

int kasumi_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
    static const u16 C[8] = { 0x0123,0x4567,0x89AB,0xCDEF, 0xFEDC,0xBA98,0x7654,0x3210 };
    u16 ukey[8], Kprime[8];
    int n;

    LTC_ARGCHK(key  != NULL);
    LTC_ARGCHK(skey != NULL);

    if (keylen != 16) {
       return CRYPT_INVALID_KEYSIZE;
    }

    if (num_rounds != 0 && num_rounds != 8) {
       return CRYPT_INVALID_ROUNDS;
    }

    /* Start by ensuring the subkeys are endian correct on a 16-bit basis */
    for (n = 0; n < 8; n++ ) {
        ukey[n] = (((u16)key[2*n]) << 8) | key[2*n+1];
    }

    /* Now build the K'[] keys */
    for (n = 0; n < 8; n++) {
        Kprime[n] = ukey[n] ^ C[n];
    }

    /* Finally construct the various sub keys */
    for(n = 0; n < 8; n++) {
        skey->kasumi.KLi1[n] = ROL16(ukey[n],1);
        skey->kasumi.KLi2[n] = Kprime[(n+2)&0x7];
        skey->kasumi.KOi1[n] = ROL16(ukey[(n+1)&0x7],5);
        skey->kasumi.KOi2[n] = ROL16(ukey[(n+5)&0x7],8);
        skey->kasumi.KOi3[n] = ROL16(ukey[(n+6)&0x7],13);
        skey->kasumi.KIi1[n] = Kprime[(n+4)&0x7];
        skey->kasumi.KIi2[n] = Kprime[(n+3)&0x7];
        skey->kasumi.KIi3[n] = Kprime[(n+7)&0x7];
    }

    return CRYPT_OK;
}

void kasumi_done(symmetric_key *skey)
{
}

int kasumi_keysize(int *keysize)
{
   LTC_ARGCHK(keysize != NULL);
   if (*keysize >= 16) {
      *keysize = 16;
      return CRYPT_OK;
   } else {
      return CRYPT_INVALID_KEYSIZE;
   }
}

int kasumi_test(void)
{
#ifndef LTC_TEST
   return CRYPT_NOP;
#else
   static const struct {
      unsigned char key[16], pt[8], ct[8];
   } tests[] = {

{
   { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
   { 0x4B, 0x58, 0xA7, 0x71, 0xAF, 0xC7, 0xE5, 0xE8 }
},

{
   { 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
   { 0x7E, 0xEF, 0x11, 0x3C, 0x95, 0xBB, 0x5A, 0x77 }
},

{
   { 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
   { 0x5F, 0x14, 0x06, 0x86, 0xD7, 0xAD, 0x5A, 0x39 },
},

{
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
   { 0x2E, 0x14, 0x91, 0xCF, 0x70, 0xAA, 0x46, 0x5D }
},

{
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
   { 0xB5, 0x45, 0x86, 0xF4, 0xAB, 0x9A, 0xE5, 0x46 }
},

};
   unsigned char buf[2][8];
   symmetric_key key;
   int err, x;

   for (x = 0; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
       if ((err = kasumi_setup(tests[x].key, 16, 0, &key)) != CRYPT_OK) {
          return err;
       }
       if ((err = kasumi_ecb_encrypt(tests[x].pt, buf[0], &key)) != CRYPT_OK) {
          return err;
       }
       if ((err = kasumi_ecb_decrypt(tests[x].ct, buf[1], &key)) != CRYPT_OK) {
          return err;
       }
       if (XMEMCMP(tests[x].pt, buf[1], 8) || XMEMCMP(tests[x].ct, buf[0], 8)) {
          return CRYPT_FAIL_TESTVECTOR;
       }
   }
   return CRYPT_OK;
#endif
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/ciphers/kasumi.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2006/12/28 01:27:23 $ */
