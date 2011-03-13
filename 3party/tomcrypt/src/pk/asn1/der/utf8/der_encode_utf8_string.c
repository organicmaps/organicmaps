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
  @file der_encode_utf8_string.c
  ASN.1 DER, encode a UTF8 STRING, Tom St Denis
*/


#ifdef LTC_DER

/**
  Store an UTF8 STRING
  @param in       The array of UTF8 to store (one per wchar_t)
  @param inlen    The number of UTF8 to store
  @param out      [out] The destination for the DER encoded UTF8 STRING
  @param outlen   [in/out] The max size and resulting size of the DER UTF8 STRING
  @return CRYPT_OK if successful
*/
int der_encode_utf8_string(const wchar_t *in,  unsigned long inlen,
                           unsigned char *out, unsigned long *outlen)
{
   unsigned long x, y, len;

   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* get the size */
   for (x = len = 0; x < inlen; x++) {
       if (in[x] < 0 || in[x] > 0x1FFFF) { 
          return CRYPT_INVALID_ARG;
       }
       len += der_utf8_charsize(in[x]);
   }

   if (len < 128) {
      y = 2 + len;
   } else if (len < 256) {
      y = 3 + len;
   } else if (len < 65536UL) {
      y = 4 + len;
   } else if (len < 16777216UL) {
      y = 5 + len;
   } else {
      return CRYPT_INVALID_ARG;
   }

   /* too big? */
   if (y > *outlen) {
      *outlen = len;
      return CRYPT_BUFFER_OVERFLOW;
   }

   /* encode the header+len */
   x = 0;
   out[x++] = 0x0C;
   if (len < 128) {
      out[x++] = (unsigned char)len;
   } else if (len < 256) {
      out[x++] = 0x81;
      out[x++] = (unsigned char)len;
   } else if (len < 65536UL) {
      out[x++] = 0x82;
      out[x++] = (unsigned char)((len>>8)&255);
      out[x++] = (unsigned char)(len&255);
   } else if (len < 16777216UL) {
      out[x++] = 0x83;
      out[x++] = (unsigned char)((len>>16)&255);
      out[x++] = (unsigned char)((len>>8)&255);
      out[x++] = (unsigned char)(len&255);
   } else {
      return CRYPT_INVALID_ARG;
   }

   /* store UTF8 */
   for (y = 0; y < inlen; y++) {
       switch (der_utf8_charsize(in[y])) {
          case 1: out[x++] = (unsigned char)in[y]; break;
          case 2: out[x++] = 0xC0 | ((in[y] >> 6) & 0x1F);  out[x++] = 0x80 | (in[y] & 0x3F); break;
          case 3: out[x++] = 0xE0 | ((in[y] >> 12) & 0x0F); out[x++] = 0x80 | ((in[y] >> 6) & 0x3F); out[x++] = 0x80 | (in[y] & 0x3F); break;
          case 4: out[x++] = 0xF0 | ((in[y] >> 18) & 0x07); out[x++] = 0x80 | ((in[y] >> 12) & 0x3F); out[x++] = 0x80 | ((in[y] >> 6) & 0x3F); out[x++] = 0x80 | (in[y] & 0x3F); break;
       }
   }

   /* retun length */
   *outlen = x;

   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utf8/der_encode_utf8_string.c,v $ */
/* $Revision: 1.9 $ */
/* $Date: 2006/12/28 01:27:24 $ */
