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
  @file rc4.c
  LTC_RC4 PRNG, Tom St Denis
*/  

#ifdef LTC_RC4

const struct ltc_prng_descriptor rc4_desc = 
{
   "rc4", 32,
    &rc4_start,
    &rc4_add_entropy,
    &rc4_ready,
    &rc4_read,
    &rc4_done,
    &rc4_export,
    &rc4_import,
    &rc4_test
};

/**
  Start the PRNG
  @param prng     [out] The PRNG state to initialize
  @return CRYPT_OK if successful
*/  
int rc4_start(prng_state *prng)
{
    LTC_ARGCHK(prng != NULL);

    /* set keysize to zero */
    prng->rc4.x = 0;
    
    return CRYPT_OK;
}

/**
  Add entropy to the PRNG state
  @param in       The data to add
  @param inlen    Length of the data to add
  @param prng     PRNG state to update
  @return CRYPT_OK if successful
*/  
int rc4_add_entropy(const unsigned char *in, unsigned long inlen, prng_state *prng)
{
    LTC_ARGCHK(in  != NULL);
    LTC_ARGCHK(prng != NULL);
 
    /* trim as required */
    if (prng->rc4.x + inlen > 256) {
       if (prng->rc4.x == 256) {
          /* I can't possibly accept another byte, ok maybe a mint wafer... */
          return CRYPT_OK;
       } else {
          /* only accept part of it */
          inlen = 256 - prng->rc4.x;
       }       
    }

    while (inlen--) {
       prng->rc4.buf[prng->rc4.x++] = *in++;
    }

    return CRYPT_OK;
    
}

/**
  Make the PRNG ready to read from
  @param prng   The PRNG to make active
  @return CRYPT_OK if successful
*/  
int rc4_ready(prng_state *prng)
{
    unsigned char key[256], tmp, *s;
    int keylen, x, y, j;

    LTC_ARGCHK(prng != NULL);

    /* extract the key */
    s = prng->rc4.buf;
    XMEMCPY(key, s, 256);
    keylen = prng->rc4.x;

    /* make LTC_RC4 perm and shuffle */
    for (x = 0; x < 256; x++) {
        s[x] = x;
    }

    for (j = x = y = 0; x < 256; x++) {
        y = (y + prng->rc4.buf[x] + key[j++]) & 255;
        if (j == keylen) {
           j = 0; 
        }
        tmp = s[x]; s[x] = s[y]; s[y] = tmp;
    }
    prng->rc4.x = 0;
    prng->rc4.y = 0;

#ifdef LTC_CLEAN_STACK
    zeromem(key, sizeof(key));
#endif

    return CRYPT_OK;
}

/**
  Read from the PRNG
  @param out      Destination
  @param outlen   Length of output
  @param prng     The active PRNG to read from
  @return Number of octets read
*/  
unsigned long rc4_read(unsigned char *out, unsigned long outlen, prng_state *prng)
{
   unsigned char x, y, *s, tmp;
   unsigned long n;

   LTC_ARGCHK(out != NULL);
   LTC_ARGCHK(prng != NULL);

#ifdef LTC_VALGRIND
   zeromem(out, outlen);
#endif

   n = outlen;
   x = prng->rc4.x;
   y = prng->rc4.y;
   s = prng->rc4.buf;
   while (outlen--) {
      x = (x + 1) & 255;
      y = (y + s[x]) & 255;
      tmp = s[x]; s[x] = s[y]; s[y] = tmp;
      tmp = (s[x] + s[y]) & 255;
      *out++ ^= s[tmp];
   }
   prng->rc4.x = x;
   prng->rc4.y = y;
   return n;
}

/**
  Terminate the PRNG
  @param prng   The PRNG to terminate
  @return CRYPT_OK if successful
*/  
int rc4_done(prng_state *prng)
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
int rc4_export(unsigned char *out, unsigned long *outlen, prng_state *prng)
{
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(prng   != NULL);

   if (*outlen < 32) {
      *outlen = 32;
      return CRYPT_BUFFER_OVERFLOW;
   }

   if (rc4_read(out, 32, prng) != 32) {
      return CRYPT_ERROR_READPRNG;
   }
   *outlen = 32;

   return CRYPT_OK;
}
 
/**
  Import a PRNG state
  @param in       The PRNG state
  @param inlen    Size of the state
  @param prng     The PRNG to import
  @return CRYPT_OK if successful
*/  
int rc4_import(const unsigned char *in, unsigned long inlen, prng_state *prng)
{
   int err;
   LTC_ARGCHK(in   != NULL);
   LTC_ARGCHK(prng != NULL);

   if (inlen != 32) {
      return CRYPT_INVALID_ARG;
   }
   
   if ((err = rc4_start(prng)) != CRYPT_OK) {
      return err;
   }
   return rc4_add_entropy(in, 32, prng);
}

/**
  PRNG self-test
  @return CRYPT_OK if successful, CRYPT_NOP if self-testing has been disabled
*/  
int rc4_test(void)
{
#if !defined(LTC_TEST) || defined(LTC_VALGRIND)
   return CRYPT_NOP;
#else
   static const struct {
      unsigned char key[8], pt[8], ct[8];
   } tests[] = {
{
   { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
   { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
   { 0x75, 0xb7, 0x87, 0x80, 0x99, 0xe0, 0xc5, 0x96 }
}
};
   prng_state prng;
   unsigned char dst[8];
   int err, x;

   for (x = 0; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
       if ((err = rc4_start(&prng)) != CRYPT_OK) {
          return err;
       }
       if ((err = rc4_add_entropy(tests[x].key, 8, &prng)) != CRYPT_OK) {
          return err;
       }
       if ((err = rc4_ready(&prng)) != CRYPT_OK) {
          return err;
       }
       XMEMCPY(dst, tests[x].pt, 8);
       if (rc4_read(dst, 8, &prng) != 8) {
          return CRYPT_ERROR_READPRNG;
       }
       rc4_done(&prng);
       if (XMEMCMP(dst, tests[x].ct, 8)) {
#if 0
          int y;
          printf("\n\nLTC_RC4 failed, I got:\n"); 
          for (y = 0; y < 8; y++) printf("%02x ", dst[y]);
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
