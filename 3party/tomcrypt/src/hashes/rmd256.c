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
   @param rmd256.c
   RLTC_MD256 Hash function
*/

#ifdef LTC_RIPEMD256

const struct ltc_hash_descriptor rmd256_desc =
{
    "rmd256",
    13,
    32,
    64,

    /* OID */
   { 1, 3, 36, 3, 2, 3 },
   6,

    &rmd256_init,
    &rmd256_process,
    &rmd256_done,
    &rmd256_test,
    NULL
};

/* the four basic functions F(), G() and H() */
#define F(x, y, z)        ((x) ^ (y) ^ (z))
#define G(x, y, z)        (((x) & (y)) | (~(x) & (z)))
#define H(x, y, z)        (((x) | ~(y)) ^ (z))
#define I(x, y, z)        (((x) & (z)) | ((y) & ~(z)))

/* the eight basic operations FF() through III() */
#define FF(a, b, c, d, x, s)        \
      (a) += F((b), (c), (d)) + (x);\
      (a) = ROLc((a), (s));

#define GG(a, b, c, d, x, s)        \
      (a) += G((b), (c), (d)) + (x) + 0x5a827999UL;\
      (a) = ROLc((a), (s));

#define HH(a, b, c, d, x, s)        \
      (a) += H((b), (c), (d)) + (x) + 0x6ed9eba1UL;\
      (a) = ROLc((a), (s));

#define II(a, b, c, d, x, s)        \
      (a) += I((b), (c), (d)) + (x) + 0x8f1bbcdcUL;\
      (a) = ROLc((a), (s));

#define FFF(a, b, c, d, x, s)        \
      (a) += F((b), (c), (d)) + (x);\
      (a) = ROLc((a), (s));

#define GGG(a, b, c, d, x, s)        \
      (a) += G((b), (c), (d)) + (x) + 0x6d703ef3UL;\
      (a) = ROLc((a), (s));

#define HHH(a, b, c, d, x, s)        \
      (a) += H((b), (c), (d)) + (x) + 0x5c4dd124UL;\
      (a) = ROLc((a), (s));

#define III(a, b, c, d, x, s)        \
      (a) += I((b), (c), (d)) + (x) + 0x50a28be6UL;\
      (a) = ROLc((a), (s));

#ifdef LTC_CLEAN_STACK
static int _rmd256_compress(hash_state *md, unsigned char *buf)
#else
static int  rmd256_compress(hash_state *md, unsigned char *buf)
#endif
{
   ulong32 aa,bb,cc,dd,aaa,bbb,ccc,ddd,tmp,X[16];
   int i;

   /* load words X */
   for (i = 0; i < 16; i++){
      LOAD32L(X[i], buf + (4 * i));
   }

   /* load state */
   aa = md->rmd256.state[0];
   bb = md->rmd256.state[1];
   cc = md->rmd256.state[2];
   dd = md->rmd256.state[3];
   aaa = md->rmd256.state[4];
   bbb = md->rmd256.state[5];
   ccc = md->rmd256.state[6];
   ddd = md->rmd256.state[7];

   /* round 1 */
   FF(aa, bb, cc, dd, X[ 0], 11);
   FF(dd, aa, bb, cc, X[ 1], 14);
   FF(cc, dd, aa, bb, X[ 2], 15);
   FF(bb, cc, dd, aa, X[ 3], 12);
   FF(aa, bb, cc, dd, X[ 4],  5);
   FF(dd, aa, bb, cc, X[ 5],  8);
   FF(cc, dd, aa, bb, X[ 6],  7);
   FF(bb, cc, dd, aa, X[ 7],  9);
   FF(aa, bb, cc, dd, X[ 8], 11);
   FF(dd, aa, bb, cc, X[ 9], 13);
   FF(cc, dd, aa, bb, X[10], 14);
   FF(bb, cc, dd, aa, X[11], 15);
   FF(aa, bb, cc, dd, X[12],  6);
   FF(dd, aa, bb, cc, X[13],  7);
   FF(cc, dd, aa, bb, X[14],  9);
   FF(bb, cc, dd, aa, X[15],  8);

   /* parallel round 1 */
   III(aaa, bbb, ccc, ddd, X[ 5],  8);
   III(ddd, aaa, bbb, ccc, X[14],  9);
   III(ccc, ddd, aaa, bbb, X[ 7],  9);
   III(bbb, ccc, ddd, aaa, X[ 0], 11);
   III(aaa, bbb, ccc, ddd, X[ 9], 13);
   III(ddd, aaa, bbb, ccc, X[ 2], 15);
   III(ccc, ddd, aaa, bbb, X[11], 15);
   III(bbb, ccc, ddd, aaa, X[ 4],  5);
   III(aaa, bbb, ccc, ddd, X[13],  7);
   III(ddd, aaa, bbb, ccc, X[ 6],  7);
   III(ccc, ddd, aaa, bbb, X[15],  8);
   III(bbb, ccc, ddd, aaa, X[ 8], 11);
   III(aaa, bbb, ccc, ddd, X[ 1], 14);
   III(ddd, aaa, bbb, ccc, X[10], 14);
   III(ccc, ddd, aaa, bbb, X[ 3], 12);
   III(bbb, ccc, ddd, aaa, X[12],  6);

   tmp = aa; aa = aaa; aaa = tmp;

   /* round 2 */
   GG(aa, bb, cc, dd, X[ 7],  7);
   GG(dd, aa, bb, cc, X[ 4],  6);
   GG(cc, dd, aa, bb, X[13],  8);
   GG(bb, cc, dd, aa, X[ 1], 13);
   GG(aa, bb, cc, dd, X[10], 11);
   GG(dd, aa, bb, cc, X[ 6],  9);
   GG(cc, dd, aa, bb, X[15],  7);
   GG(bb, cc, dd, aa, X[ 3], 15);
   GG(aa, bb, cc, dd, X[12],  7);
   GG(dd, aa, bb, cc, X[ 0], 12);
   GG(cc, dd, aa, bb, X[ 9], 15);
   GG(bb, cc, dd, aa, X[ 5],  9);
   GG(aa, bb, cc, dd, X[ 2], 11);
   GG(dd, aa, bb, cc, X[14],  7);
   GG(cc, dd, aa, bb, X[11], 13);
   GG(bb, cc, dd, aa, X[ 8], 12);

   /* parallel round 2 */
   HHH(aaa, bbb, ccc, ddd, X[ 6],  9);
   HHH(ddd, aaa, bbb, ccc, X[11], 13);
   HHH(ccc, ddd, aaa, bbb, X[ 3], 15);
   HHH(bbb, ccc, ddd, aaa, X[ 7],  7);
   HHH(aaa, bbb, ccc, ddd, X[ 0], 12);
   HHH(ddd, aaa, bbb, ccc, X[13],  8);
   HHH(ccc, ddd, aaa, bbb, X[ 5],  9);
   HHH(bbb, ccc, ddd, aaa, X[10], 11);
   HHH(aaa, bbb, ccc, ddd, X[14],  7);
   HHH(ddd, aaa, bbb, ccc, X[15],  7);
   HHH(ccc, ddd, aaa, bbb, X[ 8], 12);
   HHH(bbb, ccc, ddd, aaa, X[12],  7);
   HHH(aaa, bbb, ccc, ddd, X[ 4],  6);
   HHH(ddd, aaa, bbb, ccc, X[ 9], 15);
   HHH(ccc, ddd, aaa, bbb, X[ 1], 13);
   HHH(bbb, ccc, ddd, aaa, X[ 2], 11);

   tmp = bb; bb = bbb; bbb = tmp;

   /* round 3 */
   HH(aa, bb, cc, dd, X[ 3], 11);
   HH(dd, aa, bb, cc, X[10], 13);
   HH(cc, dd, aa, bb, X[14],  6);
   HH(bb, cc, dd, aa, X[ 4],  7);
   HH(aa, bb, cc, dd, X[ 9], 14);
   HH(dd, aa, bb, cc, X[15],  9);
   HH(cc, dd, aa, bb, X[ 8], 13);
   HH(bb, cc, dd, aa, X[ 1], 15);
   HH(aa, bb, cc, dd, X[ 2], 14);
   HH(dd, aa, bb, cc, X[ 7],  8);
   HH(cc, dd, aa, bb, X[ 0], 13);
   HH(bb, cc, dd, aa, X[ 6],  6);
   HH(aa, bb, cc, dd, X[13],  5);
   HH(dd, aa, bb, cc, X[11], 12);
   HH(cc, dd, aa, bb, X[ 5],  7);
   HH(bb, cc, dd, aa, X[12],  5);

   /* parallel round 3 */
   GGG(aaa, bbb, ccc, ddd, X[15],  9);
   GGG(ddd, aaa, bbb, ccc, X[ 5],  7);
   GGG(ccc, ddd, aaa, bbb, X[ 1], 15);
   GGG(bbb, ccc, ddd, aaa, X[ 3], 11);
   GGG(aaa, bbb, ccc, ddd, X[ 7],  8);
   GGG(ddd, aaa, bbb, ccc, X[14],  6);
   GGG(ccc, ddd, aaa, bbb, X[ 6],  6);
   GGG(bbb, ccc, ddd, aaa, X[ 9], 14);
   GGG(aaa, bbb, ccc, ddd, X[11], 12);
   GGG(ddd, aaa, bbb, ccc, X[ 8], 13);
   GGG(ccc, ddd, aaa, bbb, X[12],  5);
   GGG(bbb, ccc, ddd, aaa, X[ 2], 14);
   GGG(aaa, bbb, ccc, ddd, X[10], 13);
   GGG(ddd, aaa, bbb, ccc, X[ 0], 13);
   GGG(ccc, ddd, aaa, bbb, X[ 4],  7);
   GGG(bbb, ccc, ddd, aaa, X[13],  5);

   tmp = cc; cc = ccc; ccc = tmp;

   /* round 4 */
   II(aa, bb, cc, dd, X[ 1], 11);
   II(dd, aa, bb, cc, X[ 9], 12);
   II(cc, dd, aa, bb, X[11], 14);
   II(bb, cc, dd, aa, X[10], 15);
   II(aa, bb, cc, dd, X[ 0], 14);
   II(dd, aa, bb, cc, X[ 8], 15);
   II(cc, dd, aa, bb, X[12],  9);
   II(bb, cc, dd, aa, X[ 4],  8);
   II(aa, bb, cc, dd, X[13],  9);
   II(dd, aa, bb, cc, X[ 3], 14);
   II(cc, dd, aa, bb, X[ 7],  5);
   II(bb, cc, dd, aa, X[15],  6);
   II(aa, bb, cc, dd, X[14],  8);
   II(dd, aa, bb, cc, X[ 5],  6);
   II(cc, dd, aa, bb, X[ 6],  5);
   II(bb, cc, dd, aa, X[ 2], 12);

   /* parallel round 4 */
   FFF(aaa, bbb, ccc, ddd, X[ 8], 15);
   FFF(ddd, aaa, bbb, ccc, X[ 6],  5);
   FFF(ccc, ddd, aaa, bbb, X[ 4],  8);
   FFF(bbb, ccc, ddd, aaa, X[ 1], 11);
   FFF(aaa, bbb, ccc, ddd, X[ 3], 14);
   FFF(ddd, aaa, bbb, ccc, X[11], 14);
   FFF(ccc, ddd, aaa, bbb, X[15],  6);
   FFF(bbb, ccc, ddd, aaa, X[ 0], 14);
   FFF(aaa, bbb, ccc, ddd, X[ 5],  6);
   FFF(ddd, aaa, bbb, ccc, X[12],  9);
   FFF(ccc, ddd, aaa, bbb, X[ 2], 12);
   FFF(bbb, ccc, ddd, aaa, X[13],  9);
   FFF(aaa, bbb, ccc, ddd, X[ 9], 12);
   FFF(ddd, aaa, bbb, ccc, X[ 7],  5);
   FFF(ccc, ddd, aaa, bbb, X[10], 15);
   FFF(bbb, ccc, ddd, aaa, X[14],  8);

   tmp = dd; dd = ddd; ddd = tmp;

   /* combine results */
   md->rmd256.state[0] += aa;
   md->rmd256.state[1] += bb;
   md->rmd256.state[2] += cc;
   md->rmd256.state[3] += dd;
   md->rmd256.state[4] += aaa;
   md->rmd256.state[5] += bbb;
   md->rmd256.state[6] += ccc;
   md->rmd256.state[7] += ddd;

   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
static int rmd256_compress(hash_state *md, unsigned char *buf)
{
   int err;
   err = _rmd256_compress(md, buf);
   burn_stack(sizeof(ulong32) * 25 + sizeof(int));
   return err;
}
#endif

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int rmd256_init(hash_state * md)
{
   LTC_ARGCHK(md != NULL);
   md->rmd256.state[0] = 0x67452301UL;
   md->rmd256.state[1] = 0xefcdab89UL;
   md->rmd256.state[2] = 0x98badcfeUL;
   md->rmd256.state[3] = 0x10325476UL;
   md->rmd256.state[4] = 0x76543210UL;
   md->rmd256.state[5] = 0xfedcba98UL;
   md->rmd256.state[6] = 0x89abcdefUL;
   md->rmd256.state[7] = 0x01234567UL;
   md->rmd256.curlen   = 0;
   md->rmd256.length   = 0;
   return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(rmd256_process, rmd256_compress, rmd256, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (16 bytes)
   @return CRYPT_OK if successful
*/
int rmd256_done(hash_state * md, unsigned char *out)
{
    int i;

    LTC_ARGCHK(md  != NULL);
    LTC_ARGCHK(out != NULL);

    if (md->rmd256.curlen >= sizeof(md->rmd256.buf)) {
       return CRYPT_INVALID_ARG;
    }


    /* increase the length of the message */
    md->rmd256.length += md->rmd256.curlen * 8;

    /* append the '1' bit */
    md->rmd256.buf[md->rmd256.curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->rmd256.curlen > 56) {
        while (md->rmd256.curlen < 64) {
            md->rmd256.buf[md->rmd256.curlen++] = (unsigned char)0;
        }
        rmd256_compress(md, md->rmd256.buf);
        md->rmd256.curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->rmd256.curlen < 56) {
        md->rmd256.buf[md->rmd256.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64L(md->rmd256.length, md->rmd256.buf+56);
    rmd256_compress(md, md->rmd256.buf);

    /* copy output */
    for (i = 0; i < 8; i++) {
        STORE32L(md->rmd256.state[i], out+(4*i));
    }
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
   return CRYPT_OK;
}

/**
  Self-test the hash
  @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
*/
int rmd256_test(void)
{
#ifndef LTC_TEST
   return CRYPT_NOP;
#else
   static const struct {
        char *msg;
        unsigned char md[32];
   } tests[] = {
   { "",
     { 0x02, 0xba, 0x4c, 0x4e, 0x5f, 0x8e, 0xcd, 0x18,
       0x77, 0xfc, 0x52, 0xd6, 0x4d, 0x30, 0xe3, 0x7a,
       0x2d, 0x97, 0x74, 0xfb, 0x1e, 0x5d, 0x02, 0x63,
       0x80, 0xae, 0x01, 0x68, 0xe3, 0xc5, 0x52, 0x2d }
   },
   { "a",
     { 0xf9, 0x33, 0x3e, 0x45, 0xd8, 0x57, 0xf5, 0xd9,
       0x0a, 0x91, 0xba, 0xb7, 0x0a, 0x1e, 0xba, 0x0c,
       0xfb, 0x1b, 0xe4, 0xb0, 0x78, 0x3c, 0x9a, 0xcf,
       0xcd, 0x88, 0x3a, 0x91, 0x34, 0x69, 0x29, 0x25 }
   },
   { "abc",
     { 0xaf, 0xbd, 0x6e, 0x22, 0x8b, 0x9d, 0x8c, 0xbb,
       0xce, 0xf5, 0xca, 0x2d, 0x03, 0xe6, 0xdb, 0xa1,
       0x0a, 0xc0, 0xbc, 0x7d, 0xcb, 0xe4, 0x68, 0x0e,
       0x1e, 0x42, 0xd2, 0xe9, 0x75, 0x45, 0x9b, 0x65 }
   },
   { "message digest",
     { 0x87, 0xe9, 0x71, 0x75, 0x9a, 0x1c, 0xe4, 0x7a,
       0x51, 0x4d, 0x5c, 0x91, 0x4c, 0x39, 0x2c, 0x90,
       0x18, 0xc7, 0xc4, 0x6b, 0xc1, 0x44, 0x65, 0x55,
       0x4a, 0xfc, 0xdf, 0x54, 0xa5, 0x07, 0x0c, 0x0e }
   },
   { "abcdefghijklmnopqrstuvwxyz",
     { 0x64, 0x9d, 0x30, 0x34, 0x75, 0x1e, 0xa2, 0x16,
       0x77, 0x6b, 0xf9, 0xa1, 0x8a, 0xcc, 0x81, 0xbc,
       0x78, 0x96, 0x11, 0x8a, 0x51, 0x97, 0x96, 0x87,
       0x82, 0xdd, 0x1f, 0xd9, 0x7d, 0x8d, 0x51, 0x33 }
   },
   { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
     { 0x57, 0x40, 0xa4, 0x08, 0xac, 0x16, 0xb7, 0x20,
       0xb8, 0x44, 0x24, 0xae, 0x93, 0x1c, 0xbb, 0x1f,
       0xe3, 0x63, 0xd1, 0xd0, 0xbf, 0x40, 0x17, 0xf1,
       0xa8, 0x9f, 0x7e, 0xa6, 0xde, 0x77, 0xa0, 0xb8 }
   }
   };
   int x;
   unsigned char buf[32];
   hash_state md;

   for (x = 0; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
       rmd256_init(&md);
       rmd256_process(&md, (unsigned char *)tests[x].msg, strlen(tests[x].msg));
       rmd256_done(&md, buf);
       if (XMEMCMP(buf, tests[x].md, 32) != 0) {
       #if 0
          printf("Failed test %d\n", x);
       #endif
          return CRYPT_FAIL_TESTVECTOR;
       }
   }
   return CRYPT_OK;
#endif
}

#endif

