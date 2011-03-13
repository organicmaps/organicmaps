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

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */
#include "tomcrypt.h"

/**
  @file ecc_encrypt_key.c
  ECC Crypto, Tom St Denis
*/  

#ifdef LTC_MECC

/**
  Encrypt a symmetric key with ECC 
  @param in         The symmetric key you want to encrypt
  @param inlen      The length of the key to encrypt (octets)
  @param out        [out] The destination for the ciphertext
  @param outlen     [in/out] The max size and resulting size of the ciphertext
  @param prng       An active PRNG state
  @param wprng      The index of the PRNG you wish to use 
  @param hash       The index of the hash you want to use 
  @param key        The ECC key you want to encrypt to
  @return CRYPT_OK if successful
*/
int ecc_encrypt_key(const unsigned char *in,   unsigned long inlen,
                          unsigned char *out,  unsigned long *outlen, 
                          prng_state *prng, int wprng, int hash, 
                          ecc_key *key)
{
    unsigned char *pub_expt, *ecc_shared, *skey;
    ecc_key        pubkey;
    unsigned long  x, y, pubkeysize;
    int            err;

    LTC_ARGCHK(in      != NULL);
    LTC_ARGCHK(out     != NULL);
    LTC_ARGCHK(outlen  != NULL);
    LTC_ARGCHK(key     != NULL);

    /* check that wprng/cipher/hash are not invalid */
    if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
       return err;
    }

    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
       return err;
    }

    if (inlen > hash_descriptor[hash].hashsize) {
       return CRYPT_INVALID_HASH;
    }

    /* make a random key and export the public copy */
    if ((err = ecc_make_key_ex(prng, wprng, &pubkey, key->dp)) != CRYPT_OK) {
       return err;
    }

    pub_expt   = XMALLOC(ECC_BUF_SIZE);
    ecc_shared = XMALLOC(ECC_BUF_SIZE);
    skey       = XMALLOC(MAXBLOCKSIZE);
    if (pub_expt == NULL || ecc_shared == NULL || skey == NULL) {
       if (pub_expt != NULL) {
          XFREE(pub_expt);
       }
       if (ecc_shared != NULL) {
          XFREE(ecc_shared);
       }
       if (skey != NULL) {
          XFREE(skey);
       }
       ecc_free(&pubkey);
       return CRYPT_MEM;
    }

    pubkeysize = ECC_BUF_SIZE;
    if ((err = ecc_export(pub_expt, &pubkeysize, PK_PUBLIC, &pubkey)) != CRYPT_OK) {
       ecc_free(&pubkey);
       goto LBL_ERR;
    }
    
    /* make random key */
    x        = ECC_BUF_SIZE;
    if ((err = ecc_shared_secret(&pubkey, key, ecc_shared, &x)) != CRYPT_OK) {
       ecc_free(&pubkey);
       goto LBL_ERR;
    }
    ecc_free(&pubkey);
    y = MAXBLOCKSIZE;
    if ((err = hash_memory(hash, ecc_shared, x, skey, &y)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    
    /* Encrypt key */
    for (x = 0; x < inlen; x++) {
      skey[x] ^= in[x];
    }

    err = der_encode_sequence_multi(out, outlen,
                                    LTC_ASN1_OBJECT_IDENTIFIER,  hash_descriptor[hash].OIDlen,   hash_descriptor[hash].OID,
                                    LTC_ASN1_OCTET_STRING,       pubkeysize,                     pub_expt,
                                    LTC_ASN1_OCTET_STRING,       inlen,                          skey,
                                    LTC_ASN1_EOL,                0UL,                            NULL);

LBL_ERR:
#ifdef LTC_CLEAN_STACK
    /* clean up */
    zeromem(pub_expt,   ECC_BUF_SIZE);
    zeromem(ecc_shared, ECC_BUF_SIZE);
    zeromem(skey,       MAXBLOCKSIZE);
#endif

    XFREE(skey);
    XFREE(ecc_shared);
    XFREE(pub_expt);

    return err;
}

#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_encrypt_key.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */

