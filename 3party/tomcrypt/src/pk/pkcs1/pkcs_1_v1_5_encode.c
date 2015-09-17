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

/*! \file pkcs_1_v1_5_encode.c
 *
 *  PKCS #1 v1.5 Padding (Andreas Lange)
 */

#ifdef LTC_PKCS_1

/*! \brief PKCS #1 v1.5 encode.
 *
 *  \param msg              The data to encode
 *  \param msglen           The length of the data to encode (octets)
 *  \param block_type       Block type to use in padding (\sa ltc_pkcs_1_v1_5_blocks)
 *  \param modulus_bitlen   The bit length of the RSA modulus
 *  \param prng             An active PRNG state (only for LTC_PKCS_1_EME)
 *  \param prng_idx         The index of the PRNG desired (only for LTC_PKCS_1_EME)
 *  \param out              [out] The destination for the encoded data
 *  \param outlen           [in/out] The max size and resulting size of the encoded data
 *
 *  \return CRYPT_OK if successful
 */
int pkcs_1_v1_5_encode(const unsigned char *msg,
                             unsigned long  msglen,
                                       int  block_type,
                             unsigned long  modulus_bitlen,
                                prng_state *prng,
                                       int  prng_idx,
                             unsigned char *out,
                             unsigned long *outlen)
{
  unsigned long modulus_len, ps_len, i;
  unsigned char *ps;
  int result;

  /* valid block_type? */
  if ((block_type != LTC_PKCS_1_EMSA) &&
      (block_type != LTC_PKCS_1_EME)) {
     return CRYPT_PK_INVALID_PADDING;
  }

  if (block_type == LTC_PKCS_1_EME) {    /* encryption padding, we need a valid PRNG */
    if ((result = prng_is_valid(prng_idx)) != CRYPT_OK) {
       return result;
    }
  }

  modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

  /* test message size */
  if ((msglen + 11) > modulus_len) {
    return CRYPT_PK_INVALID_SIZE;
  }

  if (*outlen < modulus_len) {
    *outlen = modulus_len;
    result = CRYPT_BUFFER_OVERFLOW;
    goto bail;
  }

  /* generate an octets string PS */
  ps = &out[2];
  ps_len = modulus_len - msglen - 3;

  if (block_type == LTC_PKCS_1_EME) {
    /* now choose a random ps */
    if (prng_descriptor[prng_idx].read(ps, ps_len, prng) != ps_len) {
      result = CRYPT_ERROR_READPRNG;
      goto bail;
    }

    /* transform zero bytes (if any) to non-zero random bytes */
    for (i = 0; i < ps_len; i++) {
      while (ps[i] == 0) {
        if (prng_descriptor[prng_idx].read(&ps[i], 1, prng) != 1) {
          result = CRYPT_ERROR_READPRNG;
          goto bail;
        }
      }
    }
  } else {
    XMEMSET(ps, 0xFF, ps_len);
  }

  /* create string of length modulus_len */
  out[0]          = 0x00;
  out[1]          = (unsigned char)block_type;  /* block_type 1 or 2 */
  out[2 + ps_len] = 0x00;
  XMEMCPY(&out[2 + ps_len + 1], msg, msglen);
  *outlen = modulus_len;

  result  = CRYPT_OK;
bail:
  return result;
} /* pkcs_1_v1_5_encode */

#endif /* #ifdef LTC_PKCS_1 */

/* $Source$ */
/* $Revision$ */
/* $Date$ */
