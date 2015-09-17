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
  @file katja_decrypt_key.c
  Katja PKCS #1 OAEP Decryption, Tom St Denis
*/

#ifdef LTC_MKAT

/**
   (PKCS #1 v2.0) decrypt then OAEP depad
   @param in          The ciphertext
   @param inlen       The length of the ciphertext (octets)
   @param out         [out] The plaintext
   @param outlen      [in/out] The max size and resulting size of the plaintext (octets)
   @param lparam      The system "lparam" value
   @param lparamlen   The length of the lparam value (octets)
   @param hash_idx    The index of the hash desired
   @param stat        [out] Result of the decryption, 1==valid, 0==invalid
   @param key         The corresponding private Katja key
   @return CRYPT_OK if succcessul (even if invalid)
*/
int katja_decrypt_key(const unsigned char *in,       unsigned long  inlen,
                          unsigned char *out,      unsigned long *outlen,
                    const unsigned char *lparam,   unsigned long  lparamlen,
                          int            hash_idx, int           *stat,
                          katja_key       *key)
{
  unsigned long modulus_bitlen, modulus_bytelen, x;
  int           err;
  unsigned char *tmp;

  LTC_ARGCHK(out    != NULL);
  LTC_ARGCHK(outlen != NULL);
  LTC_ARGCHK(key    != NULL);
  LTC_ARGCHK(stat   != NULL);

  /* default to invalid */
  *stat = 0;

  /* valid hash ? */
  if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
     return err;
  }

  /* get modulus len in bits */
  modulus_bitlen = mp_count_bits( (key->N));

  /* payload is upto pq, so we know q is 1/3rd the size of N and therefore pq is 2/3th the size */
 modulus_bitlen = ((modulus_bitlen << 1) / 3);

  /* round down to next byte */
  modulus_bitlen -= (modulus_bitlen & 7) + 8;

  /* outlen must be at least the size of the modulus */
  modulus_bytelen = mp_unsigned_bin_size( (key->N));
  if (modulus_bytelen != inlen) {
     return CRYPT_INVALID_PACKET;
  }

  /* allocate ram */
  tmp = XMALLOC(inlen);
  if (tmp == NULL) {
     return CRYPT_MEM;
  }

  /* rsa decode the packet */
  x = inlen;
  if ((err = katja_exptmod(in, inlen, tmp, &x, PK_PRIVATE, key)) != CRYPT_OK) {
     XFREE(tmp);
     return err;
  }

  /* shift right by modulus_bytelen - modulus_bitlen/8  bytes */
  for (x = 0; x < (modulus_bitlen >> 3); x++) {
     tmp[x] = tmp[x+(modulus_bytelen-(modulus_bitlen>>3))];
  }

  /* now OAEP decode the packet */
  err = pkcs_1_oaep_decode(tmp, x, lparam, lparamlen, modulus_bitlen, hash_idx,
                           out, outlen, stat);

  XFREE(tmp);
  return err;
}

#endif /* LTC_MRSA */





/* $Source$ */
/* $Revision$ */
/* $Date$ */
