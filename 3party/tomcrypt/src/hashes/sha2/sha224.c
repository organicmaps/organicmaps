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
   @param sha224.c
   LTC_SHA-224 new NIST standard based off of LTC_SHA-256 truncated to 224 bits (Tom St Denis)
*/

const struct ltc_hash_descriptor sha224_desc =
{
    "sha224",
    10,
    28,
    64,

    /* OID */
   { 2, 16, 840, 1, 101, 3, 4, 2, 4,  },
   9,

    &sha224_init,
    &sha256_process,
    &sha224_done,
    &sha224_test,
    NULL
};

/* init the sha256 er... sha224 state ;-) */
/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int sha224_init(hash_state * md)
{
    LTC_ARGCHK(md != NULL);

    md->sha256.curlen = 0;
    md->sha256.length = 0;
    md->sha256.state[0] = 0xc1059ed8UL;
    md->sha256.state[1] = 0x367cd507UL;
    md->sha256.state[2] = 0x3070dd17UL;
    md->sha256.state[3] = 0xf70e5939UL;
    md->sha256.state[4] = 0xffc00b31UL;
    md->sha256.state[5] = 0x68581511UL;
    md->sha256.state[6] = 0x64f98fa7UL;
    md->sha256.state[7] = 0xbefa4fa4UL;
    return CRYPT_OK;
}

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (28 bytes)
   @return CRYPT_OK if successful
*/
int sha224_done(hash_state * md, unsigned char *out)
{
    unsigned char buf[32];
    int err;

    LTC_ARGCHK(md  != NULL);
    LTC_ARGCHK(out != NULL);

    err = sha256_done(md, buf);
    XMEMCPY(out, buf, 28);
#ifdef LTC_CLEAN_STACK
    zeromem(buf, sizeof(buf));
#endif 
    return err;
}

/**
  Self-test the hash
  @return CRYPT_OK if successful, CRYPT_NOP if self-tests have been disabled
*/  
int  sha224_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else    
  static const struct {
      char *msg;
      unsigned char hash[28];
  } tests[] = {
    { "abc",
      { 0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8,
        0x22, 0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2,
        0x55, 0xb3, 0x2a, 0xad, 0xbc, 0xe4, 0xbd,
        0xa0, 0xb3, 0xf7, 0xe3, 0x6c, 0x9d, 0xa7 }
    },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
      { 0x75, 0x38, 0x8b, 0x16, 0x51, 0x27, 0x76,
        0xcc, 0x5d, 0xba, 0x5d, 0xa1, 0xfd, 0x89,
        0x01, 0x50, 0xb0, 0xc6, 0x45, 0x5c, 0xb4,
        0xf5, 0x8b, 0x19, 0x52, 0x52, 0x25, 0x25 }
    },
  };

  int i;
  unsigned char tmp[28];
  hash_state md;

  for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
      sha224_init(&md);
      sha224_process(&md, (unsigned char*)tests[i].msg, (unsigned long)strlen(tests[i].msg));
      sha224_done(&md, tmp);
      if (XMEMCMP(tmp, tests[i].hash, 28) != 0) {
         return CRYPT_FAIL_TESTVECTOR;
      }
  }
  return CRYPT_OK;
 #endif
}


/* $Source: /cvs/libtom/libtomcrypt/src/hashes/sha2/sha224.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:25:28 $ */
