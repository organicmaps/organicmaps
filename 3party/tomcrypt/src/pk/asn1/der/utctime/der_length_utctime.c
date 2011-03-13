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
  @file der_length_utctime.c
  ASN.1 DER, get length of UTCTIME, Tom St Denis
*/

#ifdef LTC_DER

/**
  Gets length of DER encoding of UTCTIME
  @param utctime      The UTC time structure to get the size of
  @param outlen [out] The length of the DER encoding
  @return CRYPT_OK if successful
*/
int der_length_utctime(ltc_utctime *utctime, unsigned long *outlen)
{
   LTC_ARGCHK(outlen  != NULL);
   LTC_ARGCHK(utctime != NULL);

   if (utctime->off_hh == 0 && utctime->off_mm == 0) {
      /* we encode as YYMMDDhhmmssZ */
      *outlen = 2 + 13;
   } else {
      /* we encode as YYMMDDhhmmss{+|-}hh'mm' */
      *outlen = 2 + 17;
   }

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utctime/der_length_utctime.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */
