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
  @file chc.c
  CHC support. (Tom St Denis)
*/

#ifdef LTC_CHC_HASH

#define UNDEFED_HASH  -17

/* chc settings */
static int            cipher_idx=UNDEFED_HASH,        /* which cipher */
                      cipher_blocksize;               /* blocksize of cipher */


const struct ltc_hash_descriptor chc_desc = {
   "chc_hash", 12, 0, 0, { 0 }, 0,
   &chc_init,
   &chc_process,
   &chc_done,
   &chc_test,
   NULL
};

/**
  Initialize the CHC state with a given cipher
  @param cipher  The index of the cipher you wish to bind
  @return CRYPT_OK if successful
*/
int chc_register(int cipher)
{
   int err, kl, idx;

   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

   /* will it be valid? */
   kl = cipher_descriptor[cipher].block_length;

   /* must be >64 bit block */
   if (kl <= 8) {
      return CRYPT_INVALID_CIPHER;
   }

   /* can we use the ideal keysize? */
   if ((err = cipher_descriptor[cipher].keysize(&kl)) != CRYPT_OK) {
      return err;
   }
   /* we require that key size == block size be a valid choice */
   if (kl != cipher_descriptor[cipher].block_length) {
      return CRYPT_INVALID_CIPHER;
   }

   /* determine if chc_hash has been register_hash'ed already */
   if ((err = hash_is_valid(idx = find_hash("chc_hash"))) != CRYPT_OK) {
      return err;
   }

   /* store into descriptor */
   hash_descriptor[idx].hashsize  =
   hash_descriptor[idx].blocksize = cipher_descriptor[cipher].block_length;

   /* store the idx and block size */
   cipher_idx       = cipher;
   cipher_blocksize = cipher_descriptor[cipher].block_length;
   return CRYPT_OK;
}

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int chc_init(hash_state *md)
{
   symmetric_key *key;
   unsigned char  buf[MAXBLOCKSIZE];
   int            err;

   LTC_ARGCHK(md != NULL);

   /* is the cipher valid? */
   if ((err = cipher_is_valid(cipher_idx)) != CRYPT_OK) {
      return err;
   }

   if (cipher_blocksize != cipher_descriptor[cipher_idx].block_length) {
      return CRYPT_INVALID_CIPHER;
   }

   if ((key = XMALLOC(sizeof(*key))) == NULL) {
      return CRYPT_MEM;
   }

   /* zero key and what not */
   zeromem(buf, cipher_blocksize);
   if ((err = cipher_descriptor[cipher_idx].setup(buf, cipher_blocksize, 0, key)) != CRYPT_OK) {
      XFREE(key);
      return err;
   }

   /* encrypt zero block */
   cipher_descriptor[cipher_idx].ecb_encrypt(buf, md->chc.state, key);

   /* zero other members */
   md->chc.length = 0;
   md->chc.curlen = 0;
   zeromem(md->chc.buf, sizeof(md->chc.buf));
   XFREE(key);
   return CRYPT_OK;
}

/*
   key    <= state
   T0,T1  <= block
   T0     <= encrypt T0
   state  <= state xor T0 xor T1
*/
static int chc_compress(hash_state *md, unsigned char *buf)
{
   unsigned char  T[2][MAXBLOCKSIZE];
   symmetric_key *key;
   int            err, x;

   if ((key = XMALLOC(sizeof(*key))) == NULL) {
      return CRYPT_MEM;
   }
   if ((err = cipher_descriptor[cipher_idx].setup(md->chc.state, cipher_blocksize, 0, key)) != CRYPT_OK) {
      XFREE(key);
      return err;
   }
   XMEMCPY(T[1], buf, cipher_blocksize);
   cipher_descriptor[cipher_idx].ecb_encrypt(buf, T[0], key);
   for (x = 0; x < cipher_blocksize; x++) {
       md->chc.state[x] ^= T[0][x] ^ T[1][x];
   }
   XFREE(key);
#ifdef LTC_CLEAN_STACK
   zeromem(T, sizeof(T));
   zeromem(&key, sizeof(key));
#endif
   return CRYPT_OK;
}

/* function for processing blocks */
int _chc_process(hash_state * md, const unsigned char *buf, unsigned long len);
HASH_PROCESS(_chc_process, chc_compress, chc, (unsigned long)cipher_blocksize)

/**
   Process a block of memory though the hash
   @param md   The hash state
   @param in   The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
int chc_process(hash_state * md, const unsigned char *in, unsigned long inlen)
{
   int err;

   LTC_ARGCHK(md   != NULL);
   LTC_ARGCHK(in  != NULL);

   /* is the cipher valid? */
   if ((err = cipher_is_valid(cipher_idx)) != CRYPT_OK) {
      return err;
   }
   if (cipher_blocksize != cipher_descriptor[cipher_idx].block_length) {
      return CRYPT_INVALID_CIPHER;
   }

   return _chc_process(md, in, inlen);
}

/**
   Terminate the hash to get the digest
   @param md   The hash state
   @param out [out] The destination of the hash (length of the block size of the block cipher)
   @return CRYPT_OK if successful
*/
int chc_done(hash_state *md, unsigned char *out)
{
    int err;

    LTC_ARGCHK(md   != NULL);
    LTC_ARGCHK(out  != NULL);

    /* is the cipher valid? */
    if ((err = cipher_is_valid(cipher_idx)) != CRYPT_OK) {
       return err;
    }
    if (cipher_blocksize != cipher_descriptor[cipher_idx].block_length) {
       return CRYPT_INVALID_CIPHER;
    }

    if (md->chc.curlen >= sizeof(md->chc.buf)) {
       return CRYPT_INVALID_ARG;
    }

    /* increase the length of the message */
    md->chc.length += md->chc.curlen * 8;

    /* append the '1' bit */
    md->chc.buf[md->chc.curlen++] = (unsigned char)0x80;

    /* if the length is currently above l-8 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->chc.curlen > (unsigned long)(cipher_blocksize - 8)) {
        while (md->chc.curlen < (unsigned long)cipher_blocksize) {
            md->chc.buf[md->chc.curlen++] = (unsigned char)0;
        }
        chc_compress(md, md->chc.buf);
        md->chc.curlen = 0;
    }

    /* pad upto l-8 bytes of zeroes */
    while (md->chc.curlen < (unsigned long)(cipher_blocksize - 8)) {
        md->chc.buf[md->chc.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64L(md->chc.length, md->chc.buf+(cipher_blocksize-8));
    chc_compress(md, md->chc.buf);

    /* copy output */
    XMEMCPY(out, md->chc.state, cipher_blocksize);

#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    return CRYPT_OK;
}

/**
  Self-test the hash
  @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
*/
int chc_test(void)
{
#ifndef LTC_TEST
   return CRYPT_NOP;
#else
   static const struct {
      unsigned char *msg,
                     md[MAXBLOCKSIZE];
      int            len;
   } tests[] = {
{
   (unsigned char *)"hello world",
   { 0xcf, 0x57, 0x9d, 0xc3, 0x0a, 0x0e, 0xea, 0x61,
     0x0d, 0x54, 0x47, 0xc4, 0x3c, 0x06, 0xf5, 0x4e },
   16
}
};
   int x, oldhashidx, idx;
   unsigned char out[MAXBLOCKSIZE];
   hash_state md;

   /* AES can be under rijndael or aes... try to find it */
   if ((idx = find_cipher("aes")) == -1) {
      if ((idx = find_cipher("rijndael")) == -1) {
         return CRYPT_NOP;
      }
   }
   oldhashidx = cipher_idx;
   chc_register(idx);

   for (x = 0; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
       chc_init(&md);
       chc_process(&md, tests[x].msg, strlen((char *)tests[x].msg));
       chc_done(&md, out);
       if (XMEMCMP(out, tests[x].md, tests[x].len)) {
          return CRYPT_FAIL_TESTVECTOR;
       }
   }
   if (oldhashidx != UNDEFED_HASH) {
      chc_register(oldhashidx);
   }

   return CRYPT_OK;
#endif
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
