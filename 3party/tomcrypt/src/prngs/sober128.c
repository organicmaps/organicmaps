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
 @file sober128.c
 Implementation of SOBER-128 by Tom St Denis.
 Based on s128fast.c reference code supplied by Greg Rose of QUALCOMM.
*/

#ifdef LTC_SOBER128

#define __LTC_SOBER128TAB_C__
#include "sober128tab.c"

const struct ltc_prng_descriptor sober128_desc =
{
   "sober128", 64,
    &sober128_start,
    &sober128_add_entropy,
    &sober128_ready,
    &sober128_read,
    &sober128_done,
    &sober128_export,
    &sober128_import,
    &sober128_test
};

/* don't change these... */
#define N                        17
#define FOLD                      N /* how many iterations of folding to do */
#define INITKONST        0x6996c53a /* value of KONST to use during key loading */
#define KEYP                     15 /* where to insert key words */
#define FOLDP                     4 /* where to insert non-linear feedback */

#define B(x,i) ((unsigned char)(((x) >> (8*i)) & 0xFF))

static ulong32 BYTE2WORD(unsigned char *b)
{
   ulong32 t;
   LOAD32L(t, b);
   return t;
}

#define WORD2BYTE(w, b) STORE32L(b, w)

static void XORWORD(ulong32 w, unsigned char *b)
{
   ulong32 t;
   LOAD32L(t, b);
   t ^= w;
   STORE32L(t, b);
}

/* give correct offset for the current position of the register,
 * where logically R[0] is at position "zero".
 */
#define OFF(zero, i) (((zero)+(i)) % N)

/* step the LFSR */
/* After stepping, "zero" moves right one place */
#define STEP(R,z) \
    R[OFF(z,0)] = R[OFF(z,15)] ^ R[OFF(z,4)] ^ (R[OFF(z,0)] << 8) ^ Multab[(R[OFF(z,0)] >> 24) & 0xFF];

static void cycle(ulong32 *R)
{
    ulong32 t;
    int     i;

    STEP(R,0);
    t = R[0];
    for (i = 1; i < N; ++i) {
        R[i-1] = R[i];
    }
    R[N-1] = t;
}

/* Return a non-linear function of some parts of the register.
 */
#define NLFUNC(c,z) \
{ \
    t = c->R[OFF(z,0)] + c->R[OFF(z,16)]; \
    t ^= Sbox[(t >> 24) & 0xFF]; \
    t = RORc(t, 8); \
    t = ((t + c->R[OFF(z,1)]) ^ c->konst) + c->R[OFF(z,6)]; \
    t ^= Sbox[(t >> 24) & 0xFF]; \
    t = t + c->R[OFF(z,13)]; \
}

static ulong32 nltap(struct sober128_prng *c)
{
    ulong32 t;
    NLFUNC(c, 0);
    return t;
}

/**
  Start the PRNG
  @param prng     [out] The PRNG state to initialize
  @return CRYPT_OK if successful
*/
int sober128_start(prng_state *prng)
{
    int                   i;
    struct sober128_prng *c;

    LTC_ARGCHK(prng != NULL);

    c = &(prng->sober128);

    /* Register initialised to Fibonacci numbers */
    c->R[0] = 1;
    c->R[1] = 1;
    for (i = 2; i < N; ++i) {
       c->R[i] = c->R[i-1] + c->R[i-2];
    }
    c->konst = INITKONST;

    /* next add_entropy will be the key */
    c->flag  = 1;
    c->set   = 0;

    return CRYPT_OK;
}

/* Save the current register state
 */
static void s128_savestate(struct sober128_prng *c)
{
    int i;
    for (i = 0; i < N; ++i) {
        c->initR[i] = c->R[i];
    }
}

/* initialise to previously saved register state
 */
static void s128_reloadstate(struct sober128_prng *c)
{
    int i;

    for (i = 0; i < N; ++i) {
        c->R[i] = c->initR[i];
    }
}

/* Initialise "konst"
 */
static void s128_genkonst(struct sober128_prng *c)
{
    ulong32 newkonst;

    do {
       cycle(c->R);
       newkonst = nltap(c);
    } while ((newkonst & 0xFF000000) == 0);
    c->konst = newkonst;
}

/* Load key material into the register
 */
#define ADDKEY(k) \
   c->R[KEYP] += (k);

#define XORNL(nl) \
   c->R[FOLDP] ^= (nl);

/* nonlinear diffusion of register for key */
#define DROUND(z) STEP(c->R,z); NLFUNC(c,(z+1)); c->R[OFF((z+1),FOLDP)] ^= t;
static void s128_diffuse(struct sober128_prng *c)
{
    ulong32 t;
    /* relies on FOLD == N == 17! */
    DROUND(0);
    DROUND(1);
    DROUND(2);
    DROUND(3);
    DROUND(4);
    DROUND(5);
    DROUND(6);
    DROUND(7);
    DROUND(8);
    DROUND(9);
    DROUND(10);
    DROUND(11);
    DROUND(12);
    DROUND(13);
    DROUND(14);
    DROUND(15);
    DROUND(16);
}

/**
  Add entropy to the PRNG state
  @param in       The data to add
  @param inlen    Length of the data to add
  @param prng     PRNG state to update
  @return CRYPT_OK if successful
*/
int sober128_add_entropy(const unsigned char *in, unsigned long inlen, prng_state *prng)
{
    struct sober128_prng *c;
    ulong32               i, k;

    LTC_ARGCHK(in != NULL);
    LTC_ARGCHK(prng != NULL);
    c = &(prng->sober128);

    if (c->flag == 1) {
       /* this is the first call to the add_entropy so this input is the key */
       /* inlen must be multiple of 4 bytes */
       if ((inlen & 3) != 0) {
          return CRYPT_INVALID_KEYSIZE;
       }

       for (i = 0; i < inlen; i += 4) {
           k = BYTE2WORD((unsigned char *)&in[i]);
          ADDKEY(k);
          cycle(c->R);
          XORNL(nltap(c));
       }

       /* also fold in the length of the key */
       ADDKEY(inlen);

       /* now diffuse */
       s128_diffuse(c);

       s128_genkonst(c);
       s128_savestate(c);
       c->nbuf = 0;
       c->flag = 0;
       c->set  = 1;
    } else {
       /* ok we are adding an IV then... */
       s128_reloadstate(c);

       /* inlen must be multiple of 4 bytes */
       if ((inlen & 3) != 0) {
          return CRYPT_INVALID_KEYSIZE;
       }

       for (i = 0; i < inlen; i += 4) {
           k = BYTE2WORD((unsigned char *)&in[i]);
          ADDKEY(k);
          cycle(c->R);
          XORNL(nltap(c));
       }

       /* also fold in the length of the key */
       ADDKEY(inlen);

       /* now diffuse */
       s128_diffuse(c);
       c->nbuf = 0;
    }

    return CRYPT_OK;
}

/**
  Make the PRNG ready to read from
  @param prng   The PRNG to make active
  @return CRYPT_OK if successful
*/
int sober128_ready(prng_state *prng)
{
   return prng->sober128.set == 1 ? CRYPT_OK : CRYPT_ERROR;
}

/* XOR pseudo-random bytes into buffer
 */
#define SROUND(z) STEP(c->R,z); NLFUNC(c,(z+1)); XORWORD(t, out+(z*4));

/**
  Read from the PRNG
  @param out      Destination
  @param outlen   Length of output
  @param prng     The active PRNG to read from
  @return Number of octets read
*/
unsigned long sober128_read(unsigned char *out, unsigned long outlen, prng_state *prng)
{
   struct sober128_prng *c;
   ulong32               t, tlen;

   LTC_ARGCHK(out  != NULL);
   LTC_ARGCHK(prng != NULL);

#ifdef LTC_VALGRIND
   zeromem(out, outlen);
#endif

   c = &(prng->sober128);
   t = 0;
   tlen = outlen;

   /* handle any previously buffered bytes */
   while (c->nbuf != 0 && outlen != 0) {
      *out++ ^= c->sbuf & 0xFF;
       c->sbuf >>= 8;
       c->nbuf -= 8;
       --outlen;
   }

#ifndef LTC_SMALL_CODE
    /* do lots at a time, if there's enough to do */
    while (outlen >= N*4) {
      SROUND(0);
      SROUND(1);
      SROUND(2);
      SROUND(3);
      SROUND(4);
      SROUND(5);
      SROUND(6);
      SROUND(7);
      SROUND(8);
      SROUND(9);
      SROUND(10);
      SROUND(11);
      SROUND(12);
      SROUND(13);
      SROUND(14);
      SROUND(15);
      SROUND(16);
      out    += 4*N;
      outlen -= 4*N;
    }
#endif

    /* do small or odd size buffers the slow way */
    while (4 <= outlen) {
      cycle(c->R);
      t = nltap(c);
      XORWORD(t, out);
      out    += 4;
      outlen -= 4;
    }

    /* handle any trailing bytes */
    if (outlen != 0) {
      cycle(c->R);
      c->sbuf = nltap(c);
      c->nbuf = 32;
      while (c->nbuf != 0 && outlen != 0) {
          *out++ ^= c->sbuf & 0xFF;
          c->sbuf >>= 8;
          c->nbuf -= 8;
          --outlen;
      }
    }

    return tlen;
}

/**
  Terminate the PRNG
  @param prng   The PRNG to terminate
  @return CRYPT_OK if successful
*/
int sober128_done(prng_state *prng)
{
   LTC_ARGCHK(prng != NULL);
   return CRYPT_OK;
}

/**
  Export the PRNG state
  @param out       [out] Destination
  @param outlen    [in/out] Max size and resulting size of the state
  @param prng      The PRNG to export
  @return CRYPT_OK if successful
*/
int sober128_export(unsigned char *out, unsigned long *outlen, prng_state *prng)
{
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(prng   != NULL);

   if (*outlen < 64) {
      *outlen = 64;
      return CRYPT_BUFFER_OVERFLOW;
   }

   if (sober128_read(out, 64, prng) != 64) {
      return CRYPT_ERROR_READPRNG;
   }
   *outlen = 64;

   return CRYPT_OK;
}

/**
  Import a PRNG state
  @param in       The PRNG state
  @param inlen    Size of the state
  @param prng     The PRNG to import
  @return CRYPT_OK if successful
*/
int sober128_import(const unsigned char *in, unsigned long inlen, prng_state *prng)
{
   int err;
   LTC_ARGCHK(in   != NULL);
   LTC_ARGCHK(prng != NULL);

   if (inlen != 64) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = sober128_start(prng)) != CRYPT_OK) {
      return err;
   }
   if ((err = sober128_add_entropy(in, 64, prng)) != CRYPT_OK) {
      return err;
   }
   return sober128_ready(prng);
}

/**
  PRNG self-test
  @return CRYPT_OK if successful, CRYPT_NOP if self-testing has been disabled
*/
int sober128_test(void)
{
#ifndef LTC_TEST
   return CRYPT_NOP;
#else
   static const struct {
     int keylen, ivlen, len;
     unsigned char key[16], iv[4], out[20];
   } tests[] = {

{
   16, 4, 20,

   /* key */
   { 0x74, 0x65, 0x73, 0x74, 0x20, 0x6b, 0x65, 0x79,
     0x20, 0x31, 0x32, 0x38, 0x62, 0x69, 0x74, 0x73 },

   /* IV */
   { 0x00, 0x00, 0x00, 0x00 },

   /* expected output */
   { 0x43, 0x50, 0x0c, 0xcf, 0x89, 0x91, 0x9f, 0x1d,
     0xaa, 0x37, 0x74, 0x95, 0xf4, 0xb4, 0x58, 0xc2,
     0x40, 0x37, 0x8b, 0xbb }
}

};
   prng_state    prng;
   unsigned char dst[20];
   int           err, x;

   for (x = 0; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
       if ((err = sober128_start(&prng)) != CRYPT_OK) {
          return err;
       }
       if ((err = sober128_add_entropy(tests[x].key, tests[x].keylen, &prng)) != CRYPT_OK) {
          return err;
       }
       /* add IV */
       if ((err = sober128_add_entropy(tests[x].iv, tests[x].ivlen, &prng)) != CRYPT_OK) {
          return err;
       }

       /* ready up */
       if ((err = sober128_ready(&prng)) != CRYPT_OK) {
          return err;
       }
       XMEMSET(dst, 0, tests[x].len);
       if (sober128_read(dst, tests[x].len, &prng) != (unsigned long)tests[x].len) {
          return CRYPT_ERROR_READPRNG;
       }
       sober128_done(&prng);
       if (XMEMCMP(dst, tests[x].out, tests[x].len)) {
#if 0
          printf("\n\nLTC_SOBER128 failed, I got:\n");
          for (y = 0; y < tests[x].len; y++) printf("%02x ", dst[y]);
          printf("\n");
#endif
          return CRYPT_FAIL_TESTVECTOR;
       }
   }
   return CRYPT_OK;
#endif
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */
