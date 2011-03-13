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
  @file der_length_boolean.c
  ASN.1 DER, get length of a BOOLEAN, Tom St Denis
*/

#ifdef LTC_DER
/**
  Gets length of DER encoding of a BOOLEAN 
  @param outlen [out] The length of the DER encoding
  @return CRYPT_OK if successful
*/
int der_length_boolean(unsigned long *outlen)
{
   LTC_ARGCHK(outlen != NULL);
   *outlen = 3;
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/boolean/der_length_boolean.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2006/12/28 01:27:24 $ */
