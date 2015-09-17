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

/** @file pkcs_1_v1_5_decode.c
 *
 *  PKCS #1 v1.5 Padding. (Andreas Lange)
 */

#ifdef LTC_PKCS_1

/** @brief PKCS #1 v1.5 decode.
 *
 *  @param msg              The encoded data to decode
 *  @param msglen           The length of the encoded data (octets)
 *  @param block_type       Block type to use in padding (\sa ltc_pkcs_1_v1_5_blocks)
 *  @param modulus_bitlen   The bit length of the RSA modulus
 *  @param out              [out] Destination of decoding
 *  @param outlen           [in/out] The max size and resulting size of the decoding
 *  @param is_valid         [out] Boolean whether the padding was valid
 *
 *  @return CRYPT_OK if successful
 */
int pkcs_1_v1_5_decode(const unsigned char *msg,
                             unsigned long  msglen,
                                       int  block_type,
                             unsigned long  modulus_bitlen,
                             unsigned char *out,
                             unsigned long *outlen,
                                       int *is_valid)
{
  unsigned long modulus_len, ps_len, i;
  int result;

  /* default to invalid packet */
  *is_valid = 0;

  modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

  /* test message size */

  if ((msglen > modulus_len) || (modulus_len < 11)) {
    return CRYPT_PK_INVALID_SIZE;
  }

  result = CRYPT_OK;

  /* separate encoded message */

  if ((msg[0] != 0x00) || (msg[1] != (unsigned char)block_type)) {
    result = CRYPT_INVALID_PACKET;
  }

  if (block_type == LTC_PKCS_1_EME) {
    for (i = 2; i < modulus_len; i++) {
      /* separator */
      if (msg[i] == 0x00) { break; }
    }
    ps_len = i++ - 2;

    if (i >= modulus_len) {
      /* There was no octet with hexadecimal value 0x00 to separate ps from m.
       */
      result = CRYPT_INVALID_PACKET;
    }
  } else {
    for (i = 2; i < modulus_len - 1; i++) {
       if (msg[i] != 0xFF) { break; }
    }

    /* separator check */
    if (msg[i] != 0) {
      /* There was no octet with hexadecimal value 0x00 to separate ps from m. */
      result = CRYPT_INVALID_PACKET;
    }

    ps_len = i - 2;
  }

  if (ps_len < 8)
  {
    /* The length of ps is less than 8 octets.
     */
    result = CRYPT_INVALID_PACKET;
  }

  if (*outlen < (msglen - (2 + ps_len + 1))) {
    result = CRYPT_INVALID_PACKET;
  }

  if (result == CRYPT_OK) {
     *outlen = (msglen - (2 + ps_len + 1));
     XMEMCPY(out, &msg[2 + ps_len + 1], *outlen);

     /* valid packet */
     *is_valid = 1;
  }

  return result;
} /* pkcs_1_v1_5_decode */

#endif /* #ifdef LTC_PKCS_1 */

/* $Source$ */
/* $Revision$ */
/* $Date$ */
