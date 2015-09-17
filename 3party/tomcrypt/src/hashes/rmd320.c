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
   @file rmd320.c
   RMD320 hash function
*/

#ifdef LTC_RIPEMD320

const struct ltc_hash_descriptor rmd320_desc =
{
    "rmd320",
    14,
    40,
    64,

    /* OID ... does not exist
     * http://oid-info.com/get/1.3.36.3.2 */
   { 0 },
   0,

    &rmd320_init,
    &rmd320_process,
    &rmd320_done,
    &rmd320_test,
    NULL
};

/* the five basic functions F(), G() and H() */
#define F(x, y, z)        ((x) ^ (y) ^ (z))
#define G(x, y, z)        (((x) & (y)) | (~(x) & (z)))
#define H(x, y, z)        (((x) | ~(y)) ^ (z))
#define I(x, y, z)        (((x) & (z)) | ((y) & ~(z)))
#define J(x, y, z)        ((x) ^ ((y) | ~(z)))

/* the ten basic operations FF() through III() */
#define FF(a, b, c, d, e, x, s)        \
      (a) += F((b), (c), (d)) + (x);\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define GG(a, b, c, d, e, x, s)        \
      (a) += G((b), (c), (d)) + (x) + 0x5a827999UL;\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define HH(a, b, c, d, e, x, s)        \
      (a) += H((b), (c), (d)) + (x) + 0x6ed9eba1UL;\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define II(a, b, c, d, e, x, s)        \
      (a) += I((b), (c), (d)) + (x) + 0x8f1bbcdcUL;\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define JJ(a, b, c, d, e, x, s)        \
      (a) += J((b), (c), (d)) + (x) + 0xa953fd4eUL;\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define FFF(a, b, c, d, e, x, s)        \
      (a) += F((b), (c), (d)) + (x);\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define GGG(a, b, c, d, e, x, s)        \
      (a) += G((b), (c), (d)) + (x) + 0x7a6d76e9UL;\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define HHH(a, b, c, d, e, x, s)        \
      (a) += H((b), (c), (d)) + (x) + 0x6d703ef3UL;\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define III(a, b, c, d, e, x, s)        \
      (a) += I((b), (c), (d)) + (x) + 0x5c4dd124UL;\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);

#define JJJ(a, b, c, d, e, x, s)        \
      (a) += J((b), (c), (d)) + (x) + 0x50a28be6UL;\
      (a) = ROLc((a), (s)) + (e);\
      (c) = ROLc((c), 10);


#ifdef LTC_CLEAN_STACK
static int _rmd320_compress(hash_state *md, unsigned char *buf)
#else
static int  rmd320_compress(hash_state *md, unsigned char *buf)
#endif
{
   ulong32 aa,bb,cc,dd,ee,aaa,bbb,ccc,ddd,eee,tmp,X[16];
   int i;

   /* load words X */
   for (i = 0; i < 16; i++){
      LOAD32L(X[i], buf + (4 * i));
   }

   /* load state */
   aa = md->rmd320.state[0];
   bb = md->rmd320.state[1];
   cc = md->rmd320.state[2];
   dd = md->rmd320.state[3];
   ee = md->rmd320.state[4];
   aaa = md->rmd320.state[5];
   bbb = md->rmd320.state[6];
   ccc = md->rmd320.state[7];
   ddd = md->rmd320.state[8];
   eee = md->rmd320.state[9];

   /* round 1 */
   FF(aa, bb, cc, dd, ee, X[ 0], 11);
   FF(ee, aa, bb, cc, dd, X[ 1], 14);
   FF(dd, ee, aa, bb, cc, X[ 2], 15);
   FF(cc, dd, ee, aa, bb, X[ 3], 12);
   FF(bb, cc, dd, ee, aa, X[ 4],  5);
   FF(aa, bb, cc, dd, ee, X[ 5],  8);
   FF(ee, aa, bb, cc, dd, X[ 6],  7);
   FF(dd, ee, aa, bb, cc, X[ 7],  9);
   FF(cc, dd, ee, aa, bb, X[ 8], 11);
   FF(bb, cc, dd, ee, aa, X[ 9], 13);
   FF(aa, bb, cc, dd, ee, X[10], 14);
   FF(ee, aa, bb, cc, dd, X[11], 15);
   FF(dd, ee, aa, bb, cc, X[12],  6);
   FF(cc, dd, ee, aa, bb, X[13],  7);
   FF(bb, cc, dd, ee, aa, X[14],  9);
   FF(aa, bb, cc, dd, ee, X[15],  8);

   /* parallel round 1 */
   JJJ(aaa, bbb, ccc, ddd, eee, X[ 5],  8);
   JJJ(eee, aaa, bbb, ccc, ddd, X[14],  9);
   JJJ(ddd, eee, aaa, bbb, ccc, X[ 7],  9);
   JJJ(ccc, ddd, eee, aaa, bbb, X[ 0], 11);
   JJJ(bbb, ccc, ddd, eee, aaa, X[ 9], 13);
   JJJ(aaa, bbb, ccc, ddd, eee, X[ 2], 15);
   JJJ(eee, aaa, bbb, ccc, ddd, X[11], 15);
   JJJ(ddd, eee, aaa, bbb, ccc, X[ 4],  5);
   JJJ(ccc, ddd, eee, aaa, bbb, X[13],  7);
   JJJ(bbb, ccc, ddd, eee, aaa, X[ 6],  7);
   JJJ(aaa, bbb, ccc, ddd, eee, X[15],  8);
   JJJ(eee, aaa, bbb, ccc, ddd, X[ 8], 11);
   JJJ(ddd, eee, aaa, bbb, ccc, X[ 1], 14);
   JJJ(ccc, ddd, eee, aaa, bbb, X[10], 14);
   JJJ(bbb, ccc, ddd, eee, aaa, X[ 3], 12);
   JJJ(aaa, bbb, ccc, ddd, eee, X[12],  6);

   tmp = aa; aa = aaa; aaa = tmp;

   /* round 2 */
   GG(ee, aa, bb, cc, dd, X[ 7],  7);
   GG(dd, ee, aa, bb, cc, X[ 4],  6);
   GG(cc, dd, ee, aa, bb, X[13],  8);
   GG(bb, cc, dd, ee, aa, X[ 1], 13);
   GG(aa, bb, cc, dd, ee, X[10], 11);
   GG(ee, aa, bb, cc, dd, X[ 6],  9);
   GG(dd, ee, aa, bb, cc, X[15],  7);
   GG(cc, dd, ee, aa, bb, X[ 3], 15);
   GG(bb, cc, dd, ee, aa, X[12],  7);
   GG(aa, bb, cc, dd, ee, X[ 0], 12);
   GG(ee, aa, bb, cc, dd, X[ 9], 15);
   GG(dd, ee, aa, bb, cc, X[ 5],  9);
   GG(cc, dd, ee, aa, bb, X[ 2], 11);
   GG(bb, cc, dd, ee, aa, X[14],  7);
   GG(aa, bb, cc, dd, ee, X[11], 13);
   GG(ee, aa, bb, cc, dd, X[ 8], 12);

   /* parallel round 2 */
   III(eee, aaa, bbb, ccc, ddd, X[ 6],  9);
   III(ddd, eee, aaa, bbb, ccc, X[11], 13);
   III(ccc, ddd, eee, aaa, bbb, X[ 3], 15);
   III(bbb, ccc, ddd, eee, aaa, X[ 7],  7);
   III(aaa, bbb, ccc, ddd, eee, X[ 0], 12);
   III(eee, aaa, bbb, ccc, ddd, X[13],  8);
   III(ddd, eee, aaa, bbb, ccc, X[ 5],  9);
   III(ccc, ddd, eee, aaa, bbb, X[10], 11);
   III(bbb, ccc, ddd, eee, aaa, X[14],  7);
   III(aaa, bbb, ccc, ddd, eee, X[15],  7);
   III(eee, aaa, bbb, ccc, ddd, X[ 8], 12);
   III(ddd, eee, aaa, bbb, ccc, X[12],  7);
   III(ccc, ddd, eee, aaa, bbb, X[ 4],  6);
   III(bbb, ccc, ddd, eee, aaa, X[ 9], 15);
   III(aaa, bbb, ccc, ddd, eee, X[ 1], 13);
   III(eee, aaa, bbb, ccc, ddd, X[ 2], 11);

   tmp = bb; bb = bbb; bbb = tmp;

   /* round 3 */
   HH(dd, ee, aa, bb, cc, X[ 3], 11);
   HH(cc, dd, ee, aa, bb, X[10], 13);
   HH(bb, cc, dd, ee, aa, X[14],  6);
   HH(aa, bb, cc, dd, ee, X[ 4],  7);
   HH(ee, aa, bb, cc, dd, X[ 9], 14);
   HH(dd, ee, aa, bb, cc, X[15],  9);
   HH(cc, dd, ee, aa, bb, X[ 8], 13);
   HH(bb, cc, dd, ee, aa, X[ 1], 15);
   HH(aa, bb, cc, dd, ee, X[ 2], 14);
   HH(ee, aa, bb, cc, dd, X[ 7],  8);
   HH(dd, ee, aa, bb, cc, X[ 0], 13);
   HH(cc, dd, ee, aa, bb, X[ 6],  6);
   HH(bb, cc, dd, ee, aa, X[13],  5);
   HH(aa, bb, cc, dd, ee, X[11], 12);
   HH(ee, aa, bb, cc, dd, X[ 5],  7);
   HH(dd, ee, aa, bb, cc, X[12],  5);

   /* parallel round 3 */
   HHH(ddd, eee, aaa, bbb, ccc, X[15],  9);
   HHH(ccc, ddd, eee, aaa, bbb, X[ 5],  7);
   HHH(bbb, ccc, ddd, eee, aaa, X[ 1], 15);
   HHH(aaa, bbb, ccc, ddd, eee, X[ 3], 11);
   HHH(eee, aaa, bbb, ccc, ddd, X[ 7],  8);
   HHH(ddd, eee, aaa, bbb, ccc, X[14],  6);
   HHH(ccc, ddd, eee, aaa, bbb, X[ 6],  6);
   HHH(bbb, ccc, ddd, eee, aaa, X[ 9], 14);
   HHH(aaa, bbb, ccc, ddd, eee, X[11], 12);
   HHH(eee, aaa, bbb, ccc, ddd, X[ 8], 13);
   HHH(ddd, eee, aaa, bbb, ccc, X[12],  5);
   HHH(ccc, ddd, eee, aaa, bbb, X[ 2], 14);
   HHH(bbb, ccc, ddd, eee, aaa, X[10], 13);
   HHH(aaa, bbb, ccc, ddd, eee, X[ 0], 13);
   HHH(eee, aaa, bbb, ccc, ddd, X[ 4],  7);
   HHH(ddd, eee, aaa, bbb, ccc, X[13],  5);

   tmp = cc; cc = ccc; ccc = tmp;

   /* round 4 */
   II(cc, dd, ee, aa, bb, X[ 1], 11);
   II(bb, cc, dd, ee, aa, X[ 9], 12);
   II(aa, bb, cc, dd, ee, X[11], 14);
   II(ee, aa, bb, cc, dd, X[10], 15);
   II(dd, ee, aa, bb, cc, X[ 0], 14);
   II(cc, dd, ee, aa, bb, X[ 8], 15);
   II(bb, cc, dd, ee, aa, X[12],  9);
   II(aa, bb, cc, dd, ee, X[ 4],  8);
   II(ee, aa, bb, cc, dd, X[13],  9);
   II(dd, ee, aa, bb, cc, X[ 3], 14);
   II(cc, dd, ee, aa, bb, X[ 7],  5);
   II(bb, cc, dd, ee, aa, X[15],  6);
   II(aa, bb, cc, dd, ee, X[14],  8);
   II(ee, aa, bb, cc, dd, X[ 5],  6);
   II(dd, ee, aa, bb, cc, X[ 6],  5);
   II(cc, dd, ee, aa, bb, X[ 2], 12);

   /* parallel round 4 */
   GGG(ccc, ddd, eee, aaa, bbb, X[ 8], 15);
   GGG(bbb, ccc, ddd, eee, aaa, X[ 6],  5);
   GGG(aaa, bbb, ccc, ddd, eee, X[ 4],  8);
   GGG(eee, aaa, bbb, ccc, ddd, X[ 1], 11);
   GGG(ddd, eee, aaa, bbb, ccc, X[ 3], 14);
   GGG(ccc, ddd, eee, aaa, bbb, X[11], 14);
   GGG(bbb, ccc, ddd, eee, aaa, X[15],  6);
   GGG(aaa, bbb, ccc, ddd, eee, X[ 0], 14);
   GGG(eee, aaa, bbb, ccc, ddd, X[ 5],  6);
   GGG(ddd, eee, aaa, bbb, ccc, X[12],  9);
   GGG(ccc, ddd, eee, aaa, bbb, X[ 2], 12);
   GGG(bbb, ccc, ddd, eee, aaa, X[13],  9);
   GGG(aaa, bbb, ccc, ddd, eee, X[ 9], 12);
   GGG(eee, aaa, bbb, ccc, ddd, X[ 7],  5);
   GGG(ddd, eee, aaa, bbb, ccc, X[10], 15);
   GGG(ccc, ddd, eee, aaa, bbb, X[14],  8);

   tmp = dd; dd = ddd; ddd = tmp;

   /* round 5 */
   JJ(bb, cc, dd, ee, aa, X[ 4],  9);
   JJ(aa, bb, cc, dd, ee, X[ 0], 15);
   JJ(ee, aa, bb, cc, dd, X[ 5],  5);
   JJ(dd, ee, aa, bb, cc, X[ 9], 11);
   JJ(cc, dd, ee, aa, bb, X[ 7],  6);
   JJ(bb, cc, dd, ee, aa, X[12],  8);
   JJ(aa, bb, cc, dd, ee, X[ 2], 13);
   JJ(ee, aa, bb, cc, dd, X[10], 12);
   JJ(dd, ee, aa, bb, cc, X[14],  5);
   JJ(cc, dd, ee, aa, bb, X[ 1], 12);
   JJ(bb, cc, dd, ee, aa, X[ 3], 13);
   JJ(aa, bb, cc, dd, ee, X[ 8], 14);
   JJ(ee, aa, bb, cc, dd, X[11], 11);
   JJ(dd, ee, aa, bb, cc, X[ 6],  8);
   JJ(cc, dd, ee, aa, bb, X[15],  5);
   JJ(bb, cc, dd, ee, aa, X[13],  6);

   /* parallel round 5 */
   FFF(bbb, ccc, ddd, eee, aaa, X[12] ,  8);
   FFF(aaa, bbb, ccc, ddd, eee, X[15] ,  5);
   FFF(eee, aaa, bbb, ccc, ddd, X[10] , 12);
   FFF(ddd, eee, aaa, bbb, ccc, X[ 4] ,  9);
   FFF(ccc, ddd, eee, aaa, bbb, X[ 1] , 12);
   FFF(bbb, ccc, ddd, eee, aaa, X[ 5] ,  5);
   FFF(aaa, bbb, ccc, ddd, eee, X[ 8] , 14);
   FFF(eee, aaa, bbb, ccc, ddd, X[ 7] ,  6);
   FFF(ddd, eee, aaa, bbb, ccc, X[ 6] ,  8);
   FFF(ccc, ddd, eee, aaa, bbb, X[ 2] , 13);
   FFF(bbb, ccc, ddd, eee, aaa, X[13] ,  6);
   FFF(aaa, bbb, ccc, ddd, eee, X[14] ,  5);
   FFF(eee, aaa, bbb, ccc, ddd, X[ 0] , 15);
   FFF(ddd, eee, aaa, bbb, ccc, X[ 3] , 13);
   FFF(ccc, ddd, eee, aaa, bbb, X[ 9] , 11);
   FFF(bbb, ccc, ddd, eee, aaa, X[11] , 11);

   tmp = ee; ee = eee; eee = tmp;

   /* combine results */
   md->rmd320.state[0] += aa;
   md->rmd320.state[1] += bb;
   md->rmd320.state[2] += cc;
   md->rmd320.state[3] += dd;
   md->rmd320.state[4] += ee;
   md->rmd320.state[5] += aaa;
   md->rmd320.state[6] += bbb;
   md->rmd320.state[7] += ccc;
   md->rmd320.state[8] += ddd;
   md->rmd320.state[9] += eee;

   return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
static int rmd320_compress(hash_state *md, unsigned char *buf)
{
   int err;
   err = _rmd320_compress(md, buf);
   burn_stack(sizeof(ulong32) * 27 + sizeof(int));
   return err;
}
#endif

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int rmd320_init(hash_state * md)
{
   LTC_ARGCHK(md != NULL);
   md->rmd320.state[0] = 0x67452301UL;
   md->rmd320.state[1] = 0xefcdab89UL;
   md->rmd320.state[2] = 0x98badcfeUL;
   md->rmd320.state[3] = 0x10325476UL;
   md->rmd320.state[4] = 0xc3d2e1f0UL;
   md->rmd320.state[5] = 0x76543210UL;
   md->rmd320.state[6] = 0xfedcba98UL;
   md->rmd320.state[7] = 0x89abcdefUL;
   md->rmd320.state[8] = 0x01234567UL;
   md->rmd320.state[9] = 0x3c2d1e0fUL;
   md->rmd320.curlen   = 0;
   md->rmd320.length   = 0;
   return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(rmd320_process, rmd320_compress, rmd320, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (20 bytes)
   @return CRYPT_OK if successful
*/
int rmd320_done(hash_state * md, unsigned char *out)
{
    int i;

    LTC_ARGCHK(md  != NULL);
    LTC_ARGCHK(out != NULL);

    if (md->rmd320.curlen >= sizeof(md->rmd320.buf)) {
       return CRYPT_INVALID_ARG;
    }


    /* increase the length of the message */
    md->rmd320.length += md->rmd320.curlen * 8;

    /* append the '1' bit */
    md->rmd320.buf[md->rmd320.curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->rmd320.curlen > 56) {
        while (md->rmd320.curlen < 64) {
            md->rmd320.buf[md->rmd320.curlen++] = (unsigned char)0;
        }
        rmd320_compress(md, md->rmd320.buf);
        md->rmd320.curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->rmd320.curlen < 56) {
        md->rmd320.buf[md->rmd320.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64L(md->rmd320.length, md->rmd320.buf+56);
    rmd320_compress(md, md->rmd320.buf);

    /* copy output */
    for (i = 0; i < 10; i++) {
        STORE32L(md->rmd320.state[i], out+(4*i));
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
int rmd320_test(void)
{
#ifndef LTC_TEST
   return CRYPT_NOP;
#else
   static const struct {
        char *msg;
        unsigned char md[40];
   } tests[] = {
   { "",
     { 0x22, 0xd6, 0x5d, 0x56, 0x61, 0x53, 0x6c, 0xdc, 0x75, 0xc1,
       0xfd, 0xf5, 0xc6, 0xde, 0x7b, 0x41, 0xb9, 0xf2, 0x73, 0x25,
       0xeb, 0xc6, 0x1e, 0x85, 0x57, 0x17, 0x7d, 0x70, 0x5a, 0x0e,
       0xc8, 0x80, 0x15, 0x1c, 0x3a, 0x32, 0xa0, 0x08, 0x99, 0xb8 }
   },
   { "a",
     { 0xce, 0x78, 0x85, 0x06, 0x38, 0xf9, 0x26, 0x58, 0xa5, 0xa5,
       0x85, 0x09, 0x75, 0x79, 0x92, 0x6d, 0xda, 0x66, 0x7a, 0x57,
       0x16, 0x56, 0x2c, 0xfc, 0xf6, 0xfb, 0xe7, 0x7f, 0x63, 0x54,
       0x2f, 0x99, 0xb0, 0x47, 0x05, 0xd6, 0x97, 0x0d, 0xff, 0x5d }
   },
   { "abc",
     { 0xde, 0x4c, 0x01, 0xb3, 0x05, 0x4f, 0x89, 0x30, 0xa7, 0x9d,
       0x09, 0xae, 0x73, 0x8e, 0x92, 0x30, 0x1e, 0x5a, 0x17, 0x08,
       0x5b, 0xef, 0xfd, 0xc1, 0xb8, 0xd1, 0x16, 0x71, 0x3e, 0x74,
       0xf8, 0x2f, 0xa9, 0x42, 0xd6, 0x4c, 0xdb, 0xc4, 0x68, 0x2d }
   },
   { "message digest",
     { 0x3a, 0x8e, 0x28, 0x50, 0x2e, 0xd4, 0x5d, 0x42, 0x2f, 0x68,
       0x84, 0x4f, 0x9d, 0xd3, 0x16, 0xe7, 0xb9, 0x85, 0x33, 0xfa,
       0x3f, 0x2a, 0x91, 0xd2, 0x9f, 0x84, 0xd4, 0x25, 0xc8, 0x8d,
       0x6b, 0x4e, 0xff, 0x72, 0x7d, 0xf6, 0x6a, 0x7c, 0x01, 0x97 }
   },
   { "abcdefghijklmnopqrstuvwxyz",
     { 0xca, 0xbd, 0xb1, 0x81, 0x0b, 0x92, 0x47, 0x0a, 0x20, 0x93,
       0xaa, 0x6b, 0xce, 0x05, 0x95, 0x2c, 0x28, 0x34, 0x8c, 0xf4,
       0x3f, 0xf6, 0x08, 0x41, 0x97, 0x51, 0x66, 0xbb, 0x40, 0xed,
       0x23, 0x40, 0x04, 0xb8, 0x82, 0x44, 0x63, 0xe6, 0xb0, 0x09 }
   },
   { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
     { 0xd0, 0x34, 0xa7, 0x95, 0x0c, 0xf7, 0x22, 0x02, 0x1b, 0xa4,
       0xb8, 0x4d, 0xf7, 0x69, 0xa5, 0xde, 0x20, 0x60, 0xe2, 0x59,
       0xdf, 0x4c, 0x9b, 0xb4, 0xa4, 0x26, 0x8c, 0x0e, 0x93, 0x5b,
       0xbc, 0x74, 0x70, 0xa9, 0x69, 0xc9, 0xd0, 0x72, 0xa1, 0xac }
   }
   };
   int x;
   unsigned char buf[40];
   hash_state md;

   for (x = 0; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
       rmd320_init(&md);
       rmd320_process(&md, (unsigned char *)tests[x].msg, strlen(tests[x].msg));
       rmd320_done(&md, buf);
       if (XMEMCMP(buf, tests[x].md, 40) != 0) {
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

