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
   @file twofish.c
   Implementation of Twofish by Tom St Denis 
 */
#include "tomcrypt.h"

#ifdef LTC_TWOFISH

/* first LTC_TWOFISH_ALL_TABLES must ensure LTC_TWOFISH_TABLES is defined */
#ifdef LTC_TWOFISH_ALL_TABLES
#ifndef LTC_TWOFISH_TABLES
#define LTC_TWOFISH_TABLES
#endif
#endif

const struct ltc_cipher_descriptor twofish_desc =
{
    "twofish",
    7,
    16, 32, 16, 16,
    &twofish_setup,
    &twofish_ecb_encrypt,
    &twofish_ecb_decrypt,
    &twofish_test,
    &twofish_done,
    &twofish_keysize,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/* the two polynomials */
#define MDS_POLY          0x169
#define RS_POLY           0x14D

/* The 4x4 MDS Linear Transform */
static const unsigned char MDS[4][4] = {
    { 0x01, 0xEF, 0x5B, 0x5B },
    { 0x5B, 0xEF, 0xEF, 0x01 },
    { 0xEF, 0x5B, 0x01, 0xEF },
    { 0xEF, 0x01, 0xEF, 0x5B }
};

/* The 4x8 RS Linear Transform */
static const unsigned char RS[4][8] = {
    { 0x01, 0xA4, 0x55, 0x87, 0x5A, 0x58, 0xDB, 0x9E },
    { 0xA4, 0x56, 0x82, 0xF3, 0X1E, 0XC6, 0X68, 0XE5 },
    { 0X02, 0XA1, 0XFC, 0XC1, 0X47, 0XAE, 0X3D, 0X19 },
    { 0XA4, 0X55, 0X87, 0X5A, 0X58, 0XDB, 0X9E, 0X03 }
};

/* sbox usage orderings */
static const unsigned char qord[4][5] = {
   { 1, 1, 0, 0, 1 },
   { 0, 1, 1, 0, 0 },
   { 0, 0, 0, 1, 1 },
   { 1, 0, 1, 1, 0 }
};

#ifdef LTC_TWOFISH_TABLES

#include "twofish_tab.c"

#define sbox(i, x) ((ulong32)SBOX[i][(x)&255])

#else

/* The Q-box tables */
static const unsigned char qbox[2][4][16] = {
{
   { 0x8, 0x1, 0x7, 0xD, 0x6, 0xF, 0x3, 0x2, 0x0, 0xB, 0x5, 0x9, 0xE, 0xC, 0xA, 0x4 },
   { 0xE, 0XC, 0XB, 0X8, 0X1, 0X2, 0X3, 0X5, 0XF, 0X4, 0XA, 0X6, 0X7, 0X0, 0X9, 0XD },
   { 0XB, 0XA, 0X5, 0XE, 0X6, 0XD, 0X9, 0X0, 0XC, 0X8, 0XF, 0X3, 0X2, 0X4, 0X7, 0X1 },
   { 0XD, 0X7, 0XF, 0X4, 0X1, 0X2, 0X6, 0XE, 0X9, 0XB, 0X3, 0X0, 0X8, 0X5, 0XC, 0XA }
},
{
   { 0X2, 0X8, 0XB, 0XD, 0XF, 0X7, 0X6, 0XE, 0X3, 0X1, 0X9, 0X4, 0X0, 0XA, 0XC, 0X5 },
   { 0X1, 0XE, 0X2, 0XB, 0X4, 0XC, 0X3, 0X7, 0X6, 0XD, 0XA, 0X5, 0XF, 0X9, 0X0, 0X8 },
   { 0X4, 0XC, 0X7, 0X5, 0X1, 0X6, 0X9, 0XA, 0X0, 0XE, 0XD, 0X8, 0X2, 0XB, 0X3, 0XF },
   { 0xB, 0X9, 0X5, 0X1, 0XC, 0X3, 0XD, 0XE, 0X6, 0X4, 0X7, 0XF, 0X2, 0X0, 0X8, 0XA }
}
};

/* computes S_i[x] */
#ifdef LTC_CLEAN_STACK
static ulong32 _sbox(int i, ulong32 x)
#else
static ulong32 sbox(int i, ulong32 x)
#endif
{
   unsigned char a0,b0,a1,b1,a2,b2,a3,b3,a4,b4,y;

   /* a0,b0 = [x/16], x mod 16 */
   a0 = (unsigned char)((x>>4)&15);
   b0 = (unsigned char)((x)&15);

   /* a1 = a0 ^ b0 */
   a1 = a0 ^ b0;

   /* b1 = a0 ^ ROR(b0, 1) ^ 8a0 */
   b1 = (a0 ^ ((b0<<3)|(b0>>1)) ^ (a0<<3)) & 15;

   /* a2,b2 = t0[a1], t1[b1] */
   a2 = qbox[i][0][(int)a1];
   b2 = qbox[i][1][(int)b1];

   /* a3 = a2 ^ b2 */
   a3 = a2 ^ b2;

   /* b3 = a2 ^ ROR(b2, 1) ^ 8a2 */
   b3 = (a2 ^ ((b2<<3)|(b2>>1)) ^ (a2<<3)) & 15;

   /* a4,b4 = t2[a3], t3[b3] */
   a4 = qbox[i][2][(int)a3];
   b4 = qbox[i][3][(int)b3];

   /* y = 16b4 + a4 */
   y = (b4 << 4) + a4;

   /* return result */
   return (ulong32)y;
}

#ifdef LTC_CLEAN_STACK
static ulong32 sbox(int i, ulong32 x)
{
   ulong32 y;
   y = _sbox(i, x);
   burn_stack(sizeof(unsigned char) * 11);
   return y;
}
#endif /* LTC_CLEAN_STACK */

#endif /* LTC_TWOFISH_TABLES */

/* computes ab mod p */
static ulong32 gf_mult(ulong32 a, ulong32 b, ulong32 p)
{
   ulong32 result, B[2], P[2];

   P[1] = p;
   B[1] = b;
   result = P[0] = B[0] = 0;

   /* unrolled branchless GF multiplier */
   result ^= B[a&1]; a >>= 1;  B[1] = P[B[1]>>7] ^ (B[1] << 1); 
   result ^= B[a&1]; a >>= 1;  B[1] = P[B[1]>>7] ^ (B[1] << 1); 
   result ^= B[a&1]; a >>= 1;  B[1] = P[B[1]>>7] ^ (B[1] << 1); 
   result ^= B[a&1]; a >>= 1;  B[1] = P[B[1]>>7] ^ (B[1] << 1); 
   result ^= B[a&1]; a >>= 1;  B[1] = P[B[1]>>7] ^ (B[1] << 1); 
   result ^= B[a&1]; a >>= 1;  B[1] = P[B[1]>>7] ^ (B[1] << 1); 
   result ^= B[a&1]; a >>= 1;  B[1] = P[B[1]>>7] ^ (B[1] << 1); 
   result ^= B[a&1]; 

   return result;
}

/* computes [y0 y1 y2 y3] = MDS . [x0] */
#ifndef LTC_TWOFISH_TABLES
static ulong32 mds_column_mult(unsigned char in, int col)
{
   ulong32 x01, x5B, xEF;

   x01 = in;
   x5B = gf_mult(in, 0x5B, MDS_POLY);
   xEF = gf_mult(in, 0xEF, MDS_POLY);

   switch (col) {
       case 0:
          return (x01 << 0 ) |
                 (x5B << 8 ) |
                 (xEF << 16) |
                 (xEF << 24);
       case 1:
          return (xEF << 0 ) |
                 (xEF << 8 ) |
                 (x5B << 16) |
                 (x01 << 24);
       case 2:
          return (x5B << 0 ) |
                 (xEF << 8 ) |
                 (x01 << 16) |
                 (xEF << 24);
       case 3:
          return (x5B << 0 ) |
                 (x01 << 8 ) |
                 (xEF << 16) |
                 (x5B << 24);
   }
   /* avoid warnings, we'd never get here normally but just to calm compiler warnings... */
   return 0;
}

#else /* !LTC_TWOFISH_TABLES */

#define mds_column_mult(x, i) mds_tab[i][x]

#endif /* LTC_TWOFISH_TABLES */

/* Computes [y0 y1 y2 y3] = MDS . [x0 x1 x2 x3] */
static void mds_mult(const unsigned char *in, unsigned char *out)
{
  int x;
  ulong32 tmp;
  for (tmp = x = 0; x < 4; x++) {
      tmp ^= mds_column_mult(in[x], x);
  }
  STORE32L(tmp, out);
}

#ifdef LTC_TWOFISH_ALL_TABLES
/* computes [y0 y1 y2 y3] = RS . [x0 x1 x2 x3 x4 x5 x6 x7] */
static void rs_mult(const unsigned char *in, unsigned char *out)
{
   ulong32 tmp;
   tmp = rs_tab0[in[0]] ^ rs_tab1[in[1]] ^ rs_tab2[in[2]] ^ rs_tab3[in[3]] ^
         rs_tab4[in[4]] ^ rs_tab5[in[5]] ^ rs_tab6[in[6]] ^ rs_tab7[in[7]];
   STORE32L(tmp, out);
}

#else /* !LTC_TWOFISH_ALL_TABLES */

/* computes [y0 y1 y2 y3] = RS . [x0 x1 x2 x3 x4 x5 x6 x7] */
static void rs_mult(const unsigned char *in, unsigned char *out)
{
  int x, y;
  for (x = 0; x < 4; x++) {
      out[x] = 0;
      for (y = 0; y < 8; y++) {
          out[x] ^= gf_mult(in[y], RS[x][y], RS_POLY);
      }
  }
}

#endif

/* computes h(x) */
static void h_func(const unsigned char *in, unsigned char *out, unsigned char *M, int k, int offset)
{
  int x;
  unsigned char y[4];
  for (x = 0; x < 4; x++) {
      y[x] = in[x];
 }
  switch (k) {
     case 4:
            y[0] = (unsigned char)(sbox(1, (ulong32)y[0]) ^ M[4 * (6 + offset) + 0]);
            y[1] = (unsigned char)(sbox(0, (ulong32)y[1]) ^ M[4 * (6 + offset) + 1]);
            y[2] = (unsigned char)(sbox(0, (ulong32)y[2]) ^ M[4 * (6 + offset) + 2]);
            y[3] = (unsigned char)(sbox(1, (ulong32)y[3]) ^ M[4 * (6 + offset) + 3]);
     case 3:
            y[0] = (unsigned char)(sbox(1, (ulong32)y[0]) ^ M[4 * (4 + offset) + 0]);
            y[1] = (unsigned char)(sbox(1, (ulong32)y[1]) ^ M[4 * (4 + offset) + 1]);
            y[2] = (unsigned char)(sbox(0, (ulong32)y[2]) ^ M[4 * (4 + offset) + 2]);
            y[3] = (unsigned char)(sbox(0, (ulong32)y[3]) ^ M[4 * (4 + offset) + 3]);
     case 2:
            y[0] = (unsigned char)(sbox(1, sbox(0, sbox(0, (ulong32)y[0]) ^ M[4 * (2 + offset) + 0]) ^ M[4 * (0 + offset) + 0]));
            y[1] = (unsigned char)(sbox(0, sbox(0, sbox(1, (ulong32)y[1]) ^ M[4 * (2 + offset) + 1]) ^ M[4 * (0 + offset) + 1]));
            y[2] = (unsigned char)(sbox(1, sbox(1, sbox(0, (ulong32)y[2]) ^ M[4 * (2 + offset) + 2]) ^ M[4 * (0 + offset) + 2]));
            y[3] = (unsigned char)(sbox(0, sbox(1, sbox(1, (ulong32)y[3]) ^ M[4 * (2 + offset) + 3]) ^ M[4 * (0 + offset) + 3]));
  }
  mds_mult(y, out);
}

#ifndef LTC_TWOFISH_SMALL

/* for GCC we don't use pointer aliases */
#if defined(__GNUC__)
    #define S1 skey->twofish.S[0]
    #define S2 skey->twofish.S[1]
    #define S3 skey->twofish.S[2]
    #define S4 skey->twofish.S[3]
#endif

/* the G function */
#define g_func(x, dum)  (S1[byte(x,0)] ^ S2[byte(x,1)] ^ S3[byte(x,2)] ^ S4[byte(x,3)])
#define g1_func(x, dum) (S2[byte(x,0)] ^ S3[byte(x,1)] ^ S4[byte(x,2)] ^ S1[byte(x,3)])

#else

#ifdef LTC_CLEAN_STACK
static ulong32 _g_func(ulong32 x, symmetric_key *key)
#else
static ulong32 g_func(ulong32 x, symmetric_key *key)
#endif
{
   unsigned char g, i, y, z;
   ulong32 res;

   res = 0;
   for (y = 0; y < 4; y++) {
       z = key->twofish.start;

       /* do unkeyed substitution */
       g = sbox(qord[y][z++], (x >> (8*y)) & 255);

       /* first subkey */
       i = 0;

       /* do key mixing+sbox until z==5 */
       while (z != 5) {
          g = g ^ key->twofish.S[4*i++ + y];
          g = sbox(qord[y][z++], g);
       }

       /* multiply g by a column of the MDS */
       res ^= mds_column_mult(g, y);
   }
   return res;
}

#define g1_func(x, key) g_func(ROLc(x, 8), key)

#ifdef LTC_CLEAN_STACK
static ulong32 g_func(ulong32 x, symmetric_key *key)
{
    ulong32 y;
    y = _g_func(x, key);
    burn_stack(sizeof(unsigned char) * 4 + sizeof(ulong32));
    return y;
}
#endif /* LTC_CLEAN_STACK */

#endif /* LTC_TWOFISH_SMALL */

 /**
    Initialize the Twofish block cipher
    @param key The symmetric key you wish to pass
    @param keylen The key length in bytes
    @param num_rounds The number of rounds desired (0 for default)
    @param skey The key in as scheduled by this function.
    @return CRYPT_OK if successful
 */
#ifdef LTC_CLEAN_STACK
static int _twofish_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
#else
int twofish_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
#endif
{
#ifndef LTC_TWOFISH_SMALL
   unsigned char S[4*4], tmpx0, tmpx1;
#endif
   int k, x, y;
   unsigned char tmp[4], tmp2[4], M[8*4];
   ulong32 A, B;

   LTC_ARGCHK(key  != NULL);
   LTC_ARGCHK(skey != NULL);

   /* invalid arguments? */
   if (num_rounds != 16 && num_rounds != 0) {
      return CRYPT_INVALID_ROUNDS;
   }

   if (keylen != 16 && keylen != 24 && keylen != 32) {
      return CRYPT_INVALID_KEYSIZE;
   }

   /* k = keysize/64 [but since our keysize is in bytes...] */
   k = keylen / 8;

   /* copy the key into M */
   for (x = 0; x < keylen; x++) {
       M[x] = key[x] & 255;
   }

   /* create the S[..] words */
#ifndef LTC_TWOFISH_SMALL
   for (x = 0; x < k; x++) {
       rs_mult(M+(x*8), S+(x*4));
   }
#else
   for (x = 0; x < k; x++) {
       rs_mult(M+(x*8), skey->twofish.S+(x*4));
   }
#endif

   /* make subkeys */
   for (x = 0; x < 20; x++) {
       /* A = h(p * 2x, Me) */
       for (y = 0; y < 4; y++) {
           tmp[y] = x+x;
       }
       h_func(tmp, tmp2, M, k, 0);
       LOAD32L(A, tmp2);

       /* B = ROL(h(p * (2x + 1), Mo), 8) */
       for (y = 0; y < 4; y++) {
           tmp[y] = (unsigned char)(x+x+1);
       }
       h_func(tmp, tmp2, M, k, 1);
       LOAD32L(B, tmp2);
       B = ROLc(B, 8);

       /* K[2i]   = A + B */
       skey->twofish.K[x+x] = (A + B) & 0xFFFFFFFFUL;

       /* K[2i+1] = (A + 2B) <<< 9 */
       skey->twofish.K[x+x+1] = ROLc(B + B + A, 9);
   }

#ifndef LTC_TWOFISH_SMALL
   /* make the sboxes (large ram variant) */
   if (k == 2) {
        for (x = 0; x < 256; x++) {
           tmpx0 = (unsigned char)sbox(0, x);
           tmpx1 = (unsigned char)sbox(1, x);
           skey->twofish.S[0][x] = mds_column_mult(sbox(1, (sbox(0, tmpx0 ^ S[0]) ^ S[4])),0);
           skey->twofish.S[1][x] = mds_column_mult(sbox(0, (sbox(0, tmpx1 ^ S[1]) ^ S[5])),1);
           skey->twofish.S[2][x] = mds_column_mult(sbox(1, (sbox(1, tmpx0 ^ S[2]) ^ S[6])),2);
           skey->twofish.S[3][x] = mds_column_mult(sbox(0, (sbox(1, tmpx1 ^ S[3]) ^ S[7])),3);
        }
   } else if (k == 3) {
        for (x = 0; x < 256; x++) {
           tmpx0 = (unsigned char)sbox(0, x);
           tmpx1 = (unsigned char)sbox(1, x);
           skey->twofish.S[0][x] = mds_column_mult(sbox(1, (sbox(0, sbox(0, tmpx1 ^ S[0]) ^ S[4]) ^ S[8])),0);
           skey->twofish.S[1][x] = mds_column_mult(sbox(0, (sbox(0, sbox(1, tmpx1 ^ S[1]) ^ S[5]) ^ S[9])),1);
           skey->twofish.S[2][x] = mds_column_mult(sbox(1, (sbox(1, sbox(0, tmpx0 ^ S[2]) ^ S[6]) ^ S[10])),2);
           skey->twofish.S[3][x] = mds_column_mult(sbox(0, (sbox(1, sbox(1, tmpx0 ^ S[3]) ^ S[7]) ^ S[11])),3);
        }
   } else {
        for (x = 0; x < 256; x++) {
           tmpx0 = (unsigned char)sbox(0, x);
           tmpx1 = (unsigned char)sbox(1, x);
           skey->twofish.S[0][x] = mds_column_mult(sbox(1, (sbox(0, sbox(0, sbox(1, tmpx1 ^ S[0]) ^ S[4]) ^ S[8]) ^ S[12])),0);
           skey->twofish.S[1][x] = mds_column_mult(sbox(0, (sbox(0, sbox(1, sbox(1, tmpx0 ^ S[1]) ^ S[5]) ^ S[9]) ^ S[13])),1);
           skey->twofish.S[2][x] = mds_column_mult(sbox(1, (sbox(1, sbox(0, sbox(0, tmpx0 ^ S[2]) ^ S[6]) ^ S[10]) ^ S[14])),2);
           skey->twofish.S[3][x] = mds_column_mult(sbox(0, (sbox(1, sbox(1, sbox(0, tmpx1 ^ S[3]) ^ S[7]) ^ S[11]) ^ S[15])),3);
        }
   }
#else
   /* where to start in the sbox layers */
   /* small ram variant */
   switch (k) {
         case 4 : skey->twofish.start = 0; break;
         case 3 : skey->twofish.start = 1; break; 
         default: skey->twofish.start = 2; break;
   }
#endif
   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int twofish_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
   int x;
   x = _twofish_setup(key, keylen, num_rounds, skey);
   burn_stack(sizeof(int) * 7 + sizeof(unsigned char) * 56 + sizeof(ulong32) * 2);
   return x;
}
#endif

/**
  Encrypts a block of text with Twofish
  @param pt The input plaintext (16 bytes)
  @param ct The output ciphertext (16 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _twofish_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#else
int twofish_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
#endif
{
    ulong32 a,b,c,d,ta,tb,tc,td,t1,t2, *k;
    int r;
#if !defined(LTC_TWOFISH_SMALL) && !defined(__GNUC__)
    ulong32 *S1, *S2, *S3, *S4;
#endif    

    LTC_ARGCHK(pt   != NULL);
    LTC_ARGCHK(ct   != NULL);
    LTC_ARGCHK(skey != NULL);
    
#if !defined(LTC_TWOFISH_SMALL) && !defined(__GNUC__)
    S1 = skey->twofish.S[0];
    S2 = skey->twofish.S[1];
    S3 = skey->twofish.S[2];
    S4 = skey->twofish.S[3];
#endif    

    LOAD32L(a,&pt[0]); LOAD32L(b,&pt[4]);
    LOAD32L(c,&pt[8]); LOAD32L(d,&pt[12]);
    a ^= skey->twofish.K[0];
    b ^= skey->twofish.K[1];
    c ^= skey->twofish.K[2];
    d ^= skey->twofish.K[3];
    
    k  = skey->twofish.K + 8;
    for (r = 8; r != 0; --r) {
        t2 = g1_func(b, skey);
        t1 = g_func(a, skey) + t2;
        c  = RORc(c ^ (t1 + k[0]), 1);
        d  = ROLc(d, 1) ^ (t2 + t1 + k[1]);
        
        t2 = g1_func(d, skey);
        t1 = g_func(c, skey) + t2;
        a  = RORc(a ^ (t1 + k[2]), 1);
        b  = ROLc(b, 1) ^ (t2 + t1 + k[3]);
        k += 4;
   }

    /* output with "undo last swap" */
    ta = c ^ skey->twofish.K[4];
    tb = d ^ skey->twofish.K[5];
    tc = a ^ skey->twofish.K[6];
    td = b ^ skey->twofish.K[7];

    /* store output */
    STORE32L(ta,&ct[0]); STORE32L(tb,&ct[4]);
    STORE32L(tc,&ct[8]); STORE32L(td,&ct[12]);

    return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int twofish_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
   int err = _twofish_ecb_encrypt(pt, ct, skey);
   burn_stack(sizeof(ulong32) * 10 + sizeof(int));
   return err;
}
#endif

/**
  Decrypts a block of text with Twofish
  @param ct The input ciphertext (16 bytes)
  @param pt The output plaintext (16 bytes)
  @param skey The key as scheduled 
  @return CRYPT_OK if successful
*/
#ifdef LTC_CLEAN_STACK
static int _twofish_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#else
int twofish_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
#endif
{
    ulong32 a,b,c,d,ta,tb,tc,td,t1,t2, *k;
    int r;
#if !defined(LTC_TWOFISH_SMALL) && !defined(__GNUC__)
    ulong32 *S1, *S2, *S3, *S4;
#endif    

    LTC_ARGCHK(pt   != NULL);
    LTC_ARGCHK(ct   != NULL);
    LTC_ARGCHK(skey != NULL);
    
#if !defined(LTC_TWOFISH_SMALL) && !defined(__GNUC__)
    S1 = skey->twofish.S[0];
    S2 = skey->twofish.S[1];
    S3 = skey->twofish.S[2];
    S4 = skey->twofish.S[3];
#endif    

    /* load input */
    LOAD32L(ta,&ct[0]); LOAD32L(tb,&ct[4]);
    LOAD32L(tc,&ct[8]); LOAD32L(td,&ct[12]);

    /* undo undo final swap */
    a = tc ^ skey->twofish.K[6];
    b = td ^ skey->twofish.K[7];
    c = ta ^ skey->twofish.K[4];
    d = tb ^ skey->twofish.K[5];

    k = skey->twofish.K + 36;
    for (r = 8; r != 0; --r) {
        t2 = g1_func(d, skey);
        t1 = g_func(c, skey) + t2;
        a = ROLc(a, 1) ^ (t1 + k[2]);
        b = RORc(b ^ (t2 + t1 + k[3]), 1);

        t2 = g1_func(b, skey);
        t1 = g_func(a, skey) + t2;
        c = ROLc(c, 1) ^ (t1 + k[0]);
        d = RORc(d ^ (t2 +  t1 + k[1]), 1);
        k -= 4;
    }

    /* pre-white */
    a ^= skey->twofish.K[0];
    b ^= skey->twofish.K[1];
    c ^= skey->twofish.K[2];
    d ^= skey->twofish.K[3];
    
    /* store */
    STORE32L(a, &pt[0]); STORE32L(b, &pt[4]);
    STORE32L(c, &pt[8]); STORE32L(d, &pt[12]);
    return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
int twofish_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
   int err =_twofish_ecb_decrypt(ct, pt, skey);
   burn_stack(sizeof(ulong32) * 10 + sizeof(int));
   return err;
}
#endif

/**
  Performs a self-test of the Twofish block cipher
  @return CRYPT_OK if functional, CRYPT_NOP if self-test has been disabled
*/
int twofish_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else    
 static const struct { 
     int keylen;
     unsigned char key[32], pt[16], ct[16];
 } tests[] = {
   { 16,
     { 0x9F, 0x58, 0x9F, 0x5C, 0xF6, 0x12, 0x2C, 0x32,
       0xB6, 0xBF, 0xEC, 0x2F, 0x2A, 0xE8, 0xC3, 0x5A },
     { 0xD4, 0x91, 0xDB, 0x16, 0xE7, 0xB1, 0xC3, 0x9E,
       0x86, 0xCB, 0x08, 0x6B, 0x78, 0x9F, 0x54, 0x19 },
     { 0x01, 0x9F, 0x98, 0x09, 0xDE, 0x17, 0x11, 0x85,
       0x8F, 0xAA, 0xC3, 0xA3, 0xBA, 0x20, 0xFB, 0xC3 }
   }, {
     24,
     { 0x88, 0xB2, 0xB2, 0x70, 0x6B, 0x10, 0x5E, 0x36,
       0xB4, 0x46, 0xBB, 0x6D, 0x73, 0x1A, 0x1E, 0x88,
       0xEF, 0xA7, 0x1F, 0x78, 0x89, 0x65, 0xBD, 0x44 },
     { 0x39, 0xDA, 0x69, 0xD6, 0xBA, 0x49, 0x97, 0xD5,
       0x85, 0xB6, 0xDC, 0x07, 0x3C, 0xA3, 0x41, 0xB2 },
     { 0x18, 0x2B, 0x02, 0xD8, 0x14, 0x97, 0xEA, 0x45,
       0xF9, 0xDA, 0xAC, 0xDC, 0x29, 0x19, 0x3A, 0x65 }
   }, { 
     32,
     { 0xD4, 0x3B, 0xB7, 0x55, 0x6E, 0xA3, 0x2E, 0x46,
       0xF2, 0xA2, 0x82, 0xB7, 0xD4, 0x5B, 0x4E, 0x0D,
       0x57, 0xFF, 0x73, 0x9D, 0x4D, 0xC9, 0x2C, 0x1B,
       0xD7, 0xFC, 0x01, 0x70, 0x0C, 0xC8, 0x21, 0x6F },
     { 0x90, 0xAF, 0xE9, 0x1B, 0xB2, 0x88, 0x54, 0x4F,
       0x2C, 0x32, 0xDC, 0x23, 0x9B, 0x26, 0x35, 0xE6 },
     { 0x6C, 0xB4, 0x56, 0x1C, 0x40, 0xBF, 0x0A, 0x97,
       0x05, 0x93, 0x1C, 0xB6, 0xD4, 0x08, 0xE7, 0xFA }
   }
};


 symmetric_key key;
 unsigned char tmp[2][16];
 int err, i, y;
 
 for (i = 0; i < (int)(sizeof(tests)/sizeof(tests[0])); i++) {
    if ((err = twofish_setup(tests[i].key, tests[i].keylen, 0, &key)) != CRYPT_OK) {
       return err;
    }
    twofish_ecb_encrypt(tests[i].pt, tmp[0], &key);
    twofish_ecb_decrypt(tmp[0], tmp[1], &key);
    if (XMEMCMP(tmp[0], tests[i].ct, 16) != 0 || XMEMCMP(tmp[1], tests[i].pt, 16) != 0) {
#if 0
       printf("Twofish failed test %d, %d, %d\n", i, XMEMCMP(tmp[0], tests[i].ct, 16), XMEMCMP(tmp[1], tests[i].pt, 16));
#endif
       return CRYPT_FAIL_TESTVECTOR;
    }
      /* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
      for (y = 0; y < 16; y++) tmp[0][y] = 0;
      for (y = 0; y < 1000; y++) twofish_ecb_encrypt(tmp[0], tmp[0], &key);
      for (y = 0; y < 1000; y++) twofish_ecb_decrypt(tmp[0], tmp[0], &key);
      for (y = 0; y < 16; y++) if (tmp[0][y] != 0) return CRYPT_FAIL_TESTVECTOR;
 }    
 return CRYPT_OK;
#endif 
}

/** Terminate the context 
   @param skey    The scheduled key
*/
void twofish_done(symmetric_key *skey)
{
}

/**
  Gets suitable key size
  @param keysize [in/out] The length of the recommended key (in bytes).  This function will store the suitable size back in this variable.
  @return CRYPT_OK if the input key size is acceptable.
*/
int twofish_keysize(int *keysize)
{
   LTC_ARGCHK(keysize);
   if (*keysize < 16)
      return CRYPT_INVALID_KEYSIZE;
   if (*keysize < 24) {
      *keysize = 16;
      return CRYPT_OK;
   } else if (*keysize < 32) {
      *keysize = 24;
      return CRYPT_OK;
   } else {
      *keysize = 32;
      return CRYPT_OK;
   }
}

#endif




/* $Source: /cvs/libtom/libtomcrypt/src/ciphers/twofish/twofish.c,v $ */
/* $Revision: 1.16 $ */
/* $Date: 2007/05/12 14:32:35 $ */
