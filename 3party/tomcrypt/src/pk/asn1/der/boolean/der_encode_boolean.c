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
  @file der_encode_boolean.c
  ASN.1 DER, encode a BOOLEAN, Tom St Denis
*/


#ifdef LTC_DER

/**
  Store a BOOLEAN
  @param in       The boolean to encode
  @param out      [out] The destination for the DER encoded BOOLEAN
  @param outlen   [in/out] The max size and resulting size of the DER BOOLEAN
  @return CRYPT_OK if successful
*/
int der_encode_boolean(int in, 
                       unsigned char *out, unsigned long *outlen)
{
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(out    != NULL);
   
   if (*outlen < 3) {
       *outlen = 3;
       return CRYPT_BUFFER_OVERFLOW;
   }
   
   *outlen = 3;
   out[0] = 0x01;
   out[1] = 0x01;
   out[2] = in ? 0xFF : 0x00;
   
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/boolean/der_encode_boolean.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2006/12/28 01:27:24 $ */
