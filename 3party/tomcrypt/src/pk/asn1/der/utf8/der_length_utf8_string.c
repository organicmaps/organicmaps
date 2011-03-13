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
  @file der_length_utf8_string.c
  ASN.1 DER, get length of UTF8 STRING, Tom St Denis
*/

#ifdef LTC_DER

/** Return the size in bytes of a UTF-8 character
  @param c   The UTF-8 character to measure
  @return    The size in bytes
*/
unsigned long der_utf8_charsize(const wchar_t c)
{
   if (c <= 0x7F) {
      return 1;
   } else if (c <= 0x7FF) {
      return 2;
   } else if (c <= 0xFFFF) {
      return 3;
   } else {
      return 4;
   }
}

/**
  Gets length of DER encoding of UTF8 STRING 
  @param in       The characters to measure the length of
  @param noctets  The number of octets in the string to encode
  @param outlen   [out] The length of the DER encoding for the given string
  @return CRYPT_OK if successful
*/
int der_length_utf8_string(const wchar_t *in, unsigned long noctets, unsigned long *outlen)
{
   unsigned long x, len;

   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(outlen != NULL);

   len = 0;
   for (x = 0; x < noctets; x++) {
      if (in[x] < 0 || in[x] > 0x10FFFF) {
         return CRYPT_INVALID_ARG;
      }
      len += der_utf8_charsize(in[x]);
   }

   if (len < 128) {
      /* 0C LL DD DD DD ... */
      *outlen = 2 + len;
   } else if (len < 256) {
      /* 0C 81 LL DD DD DD ... */
      *outlen = 3 + len;
   } else if (len < 65536UL) {
      /* 0C 82 LL LL DD DD DD ... */
      *outlen = 4 + len;
   } else if (len < 16777216UL) {
      /* 0C 83 LL LL LL DD DD DD ... */
      *outlen = 5 + len;
   } else {
      return CRYPT_INVALID_ARG;
   }

   return CRYPT_OK;
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utf8/der_length_utf8_string.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:24 $ */
