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
   @param md4.c
   Submitted by Dobes Vandermeer  (dobes@smartt.com) 
*/

#ifdef LTC_MD4

const struct ltc_hash_descriptor md4_desc =
{
    "md4",
    6,
    16,
    64,
 
    /* OID */
   { 1, 2, 840, 113549, 2, 4,  },
   6,

    &md4_init,
    &md4_process,
    &md4_done,
    &md4_test,
    NULL
};

#define S11 3
#define S12 7
#define S13 11
#define S14 19
#define S21 3
#define S22 5
#define S23 9
#define S24 13
#define S31 3
#define S32 9
#define S33 11
#define S34 15

/* F, G and H are basic LTC_MD4 functions. */
#define F(x, y, z) (z ^ (x & (y ^ z)))
#define G(x, y, z) ((x & y) | (z & (x | y)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) ROLc(x, n)

/* FF, GG and HH are transformations for rounds 1, 2 and 3 */ 
/* Rotation is separate from addition to prevent recomputation */ 

#define FF(a, b, c, d, x, s) { \
    (a) += F ((b), (c), (d)) + (x); \
    (a) = ROTATE_LEFT ((a), (s)); \
  }
#define GG(a, b, c, d, x, s) { \
    (a) += G ((b), (c), (d)) + (x) + 0x5a827999UL; \
    (a) = ROTATE_LEFT ((a), (s)); \
  }
#define HH(a, b, c, d, x, s) { \
    (a) += H ((b), (c), (d)) + (x) + 0x6ed9eba1UL; \
    (a) = ROTATE_LEFT ((a), (s)); \
  }

#ifdef LTC_CLEAN_STACK
static int _md4_compress(hash_state *md, unsigned char *buf)
#else
static int  md4_compress(hash_state *md, unsigned char *buf)
#endif
{
    ulong32 x[16], a, b, c, d;
    int i;

    /* copy state */
    a = md->md4.state[0];
    b = md->md4.state[1];
    c = md->md4.state[2];
    d = md->md4.state[3];

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32L(x[i], buf + (4*i));
    }
 
    /* Round 1 */ 
    FF (a, b, c, d, x[ 0], S11); /* 1 */ 
    FF (d, a, b, c, x[ 1], S12); /* 2 */ 
    FF (c, d, a, b, x[ 2], S13); /* 3 */ 
    FF (b, c, d, a, x[ 3], S14); /* 4 */ 
    FF (a, b, c, d, x[ 4], S11); /* 5 */ 
    FF (d, a, b, c, x[ 5], S12); /* 6 */ 
    FF (c, d, a, b, x[ 6], S13); /* 7 */ 
    FF (b, c, d, a, x[ 7], S14); /* 8 */ 
    FF (a, b, c, d, x[ 8], S11); /* 9 */ 
    FF (d, a, b, c, x[ 9], S12); /* 10 */
    FF (c, d, a, b, x[10], S13); /* 11 */ 
    FF (b, c, d, a, x[11], S14); /* 12 */
    FF (a, b, c, d, x[12], S11); /* 13 */
    FF (d, a, b, c, x[13], S12); /* 14 */ 
    FF (c, d, a, b, x[14], S13); /* 15 */ 
    FF (b, c, d, a, x[15], S14); /* 16 */ 
    
    /* Round 2 */ 
    GG (a, b, c, d, x[ 0], S21); /* 17 */ 
    GG (d, a, b, c, x[ 4], S22); /* 18 */ 
    GG (c, d, a, b, x[ 8], S23); /* 19 */ 
    GG (b, c, d, a, x[12], S24); /* 20 */ 
    GG (a, b, c, d, x[ 1], S21); /* 21 */ 
    GG (d, a, b, c, x[ 5], S22); /* 22 */ 
    GG (c, d, a, b, x[ 9], S23); /* 23 */ 
    GG (b, c, d, a, x[13], S24); /* 24 */ 
    GG (a, b, c, d, x[ 2], S21); /* 25 */ 
    GG (d, a, b, c, x[ 6], S22); /* 26 */ 
    GG (c, d, a, b, x[10], S23); /* 27 */ 
    GG (b, c, d, a, x[14], S24); /* 28 */ 
    GG (a, b, c, d, x[ 3], S21); /* 29 */ 
    GG (d, a, b, c, x[ 7], S22); /* 30 */ 
    GG (c, d, a, b, x[11], S23); /* 31 */ 
    GG (b, c, d, a, x[15], S24); /* 32 */ 
    
    /* Round 3 */
    HH (a, b, c, d, x[ 0], S31); /* 33 */ 
    HH (d, a, b, c, x[ 8], S32); /* 34 */ 
    HH (c, d, a, b, x[ 4], S33); /* 35 */ 
    HH (b, c, d, a, x[12], S34); /* 36 */ 
    HH (a, b, c, d, x[ 2], S31); /* 37 */ 
    HH (d, a, b, c, x[10], S32); /* 38 */ 
    HH (c, d, a, b, x[ 6], S33); /* 39 */ 
    HH (b, c, d, a, x[14], S34); /* 40 */ 
    HH (a, b, c, d, x[ 1], S31); /* 41 */ 
    HH (d, a, b, c, x[ 9], S32); /* 42 */ 
    HH (c, d, a, b, x[ 5], S33); /* 43 */ 
    HH (b, c, d, a, x[13], S34); /* 44 */ 
    HH (a, b, c, d, x[ 3], S31); /* 45 */ 
    HH (d, a, b, c, x[11], S32); /* 46 */ 
    HH (c, d, a, b, x[ 7], S33); /* 47 */ 
    HH (b, c, d, a, x[15], S34); /* 48 */ 
    

    /* Update our state */
    md->md4.state[0] = md->md4.state[0] + a;
    md->md4.state[1] = md->md4.state[1] + b;
    md->md4.state[2] = md->md4.state[2] + c;
    md->md4.state[3] = md->md4.state[3] + d;

    return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
static int md4_compress(hash_state *md, unsigned char *buf)
{
   int err;
   err = _md4_compress(md, buf);
   burn_stack(sizeof(ulong32) * 20 + sizeof(int));
   return err;
}
#endif

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int md4_init(hash_state * md)
{
   LTC_ARGCHK(md != NULL);
   md->md4.state[0] = 0x67452301UL;
   md->md4.state[1] = 0xefcdab89UL;
   md->md4.state[2] = 0x98badcfeUL;
   md->md4.state[3] = 0x10325476UL;
   md->md4.length  = 0;
   md->md4.curlen  = 0;
   return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(md4_process, md4_compress, md4, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (16 bytes)
   @return CRYPT_OK if successful
*/
int md4_done(hash_state * md, unsigned char *out)
{
    int i;

    LTC_ARGCHK(md  != NULL);
    LTC_ARGCHK(out != NULL);

    if (md->md4.curlen >= sizeof(md->md4.buf)) {
       return CRYPT_INVALID_ARG;
    }

    /* increase the length of the message */
    md->md4.length += md->md4.curlen * 8;

    /* append the '1' bit */
    md->md4.buf[md->md4.curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->md4.curlen > 56) {
        while (md->md4.curlen < 64) {
            md->md4.buf[md->md4.curlen++] = (unsigned char)0;
        }
        md4_compress(md, md->md4.buf);
        md->md4.curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->md4.curlen < 56) {
        md->md4.buf[md->md4.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64L(md->md4.length, md->md4.buf+56);
    md4_compress(md, md->md4.buf);

    /* copy output */
    for (i = 0; i < 4; i++) {
        STORE32L(md->md4.state[i], out+(4*i));
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
int md4_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else    
    static const struct md4_test_case {
        char *input;
        unsigned char digest[16];
    } cases[] = {
        { "", 
          {0x31, 0xd6, 0xcf, 0xe0, 0xd1, 0x6a, 0xe9, 0x31,
           0xb7, 0x3c, 0x59, 0xd7, 0xe0, 0xc0, 0x89, 0xc0} },
        { "a",
          {0xbd, 0xe5, 0x2c, 0xb3, 0x1d, 0xe3, 0x3e, 0x46,
           0x24, 0x5e, 0x05, 0xfb, 0xdb, 0xd6, 0xfb, 0x24} },
        { "abc",
          {0xa4, 0x48, 0x01, 0x7a, 0xaf, 0x21, 0xd8, 0x52, 
           0x5f, 0xc1, 0x0a, 0xe8, 0x7a, 0xa6, 0x72, 0x9d} },
        { "message digest", 
          {0xd9, 0x13, 0x0a, 0x81, 0x64, 0x54, 0x9f, 0xe8, 
           0x18, 0x87, 0x48, 0x06, 0xe1, 0xc7, 0x01, 0x4b} },
        { "abcdefghijklmnopqrstuvwxyz", 
          {0xd7, 0x9e, 0x1c, 0x30, 0x8a, 0xa5, 0xbb, 0xcd, 
           0xee, 0xa8, 0xed, 0x63, 0xdf, 0x41, 0x2d, 0xa9} },
        { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", 
          {0x04, 0x3f, 0x85, 0x82, 0xf2, 0x41, 0xdb, 0x35, 
           0x1c, 0xe6, 0x27, 0xe1, 0x53, 0xe7, 0xf0, 0xe4} },
        { "12345678901234567890123456789012345678901234567890123456789012345678901234567890", 
          {0xe3, 0x3b, 0x4d, 0xdc, 0x9c, 0x38, 0xf2, 0x19, 
           0x9c, 0x3e, 0x7b, 0x16, 0x4f, 0xcc, 0x05, 0x36} },
    };
    int i;
    hash_state md;
    unsigned char digest[16];

    for(i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        md4_init(&md);
        md4_process(&md, (unsigned char *)cases[i].input, (unsigned long)strlen(cases[i].input));
        md4_done(&md, digest);
        if (XMEMCMP(digest, cases[i].digest, 16) != 0) {
           return CRYPT_FAIL_TESTVECTOR;
        }

    }
    return CRYPT_OK;
  #endif
}

#endif



/* $Source: /cvs/libtom/libtomcrypt/src/hashes/md4.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:25:28 $ */
