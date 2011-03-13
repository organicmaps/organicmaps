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
  @file der_decode_printable_string.c
  ASN.1 DER, encode a printable STRING, Tom St Denis
*/


#ifdef LTC_DER

/**
  Store a printable STRING
  @param in      The DER encoded printable STRING
  @param inlen   The size of the DER printable STRING
  @param out     [out] The array of octets stored (one per char)
  @param outlen  [in/out] The number of octets stored
  @return CRYPT_OK if successful
*/
int der_decode_printable_string(const unsigned char *in, unsigned long inlen,
                                unsigned char *out, unsigned long *outlen)
{
   unsigned long x, y, len;
   int           t;

   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* must have header at least */
   if (inlen < 2) {
      return CRYPT_INVALID_PACKET;
   }

   /* check for 0x13 */
   if ((in[0] & 0x1F) != 0x13) {
      return CRYPT_INVALID_PACKET;
   }
   x = 1;

   /* decode the length */
   if (in[x] & 0x80) {
      /* valid # of bytes in length are 1,2,3 */
      y = in[x] & 0x7F;
      if ((y == 0) || (y > 3) || ((x + y) > inlen)) {
         return CRYPT_INVALID_PACKET;
      }

      /* read the length in */
      len = 0;
      ++x;
      while (y--) {
         len = (len << 8) | in[x++];
      }
   } else {
      len = in[x++] & 0x7F;
   }

   /* is it too long? */
   if (len > *outlen) {
      *outlen = len;
      return CRYPT_BUFFER_OVERFLOW;
   }

   if (len + x > inlen) {
      return CRYPT_INVALID_PACKET;
   }

   /* read the data */
   for (y = 0; y < len; y++) {
       t = der_printable_value_decode(in[x++]);
       if (t == -1) {
           return CRYPT_INVALID_ARG;
       }
       out[y] = t;
   }

   *outlen = y;

   return CRYPT_OK;
}
 
#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/printable_string/der_decode_printable_string.c,v $ */
/* $Revision: 1.4 $ */
/* $Date: 2006/12/28 01:27:24 $ */
