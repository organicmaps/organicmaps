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
  @file der_decode_short_integer.c
  ASN.1 DER, decode an integer, Tom St Denis
*/


#ifdef LTC_DER

/**
  Read a short integer
  @param in       The DER encoded data
  @param inlen    Size of data
  @param num      [out] The integer to decode
  @return CRYPT_OK if successful
*/
int der_decode_short_integer(const unsigned char *in, unsigned long inlen, unsigned long *num)
{
   unsigned long len, x, y;

   LTC_ARGCHK(num    != NULL);
   LTC_ARGCHK(in     != NULL);

   /* check length */
   if (inlen < 2) {
      return CRYPT_INVALID_PACKET;
   }

   /* check header */
   x = 0;
   if ((in[x++] & 0x1F) != 0x02) {
      return CRYPT_INVALID_PACKET;
   }

   /* get the packet len */
   len = in[x++];

   if (x + len > inlen) {
      return CRYPT_INVALID_PACKET;
   }

   /* read number */
   y = 0;
   while (len--) {
      y = (y<<8) | (unsigned long)in[x++];
   }
   *num = y;

   return CRYPT_OK;

}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/short_integer/der_decode_short_integer.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:24 $ */
