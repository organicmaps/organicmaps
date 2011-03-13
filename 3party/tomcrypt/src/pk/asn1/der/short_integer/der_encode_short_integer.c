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
  @file der_encode_short_integer.c
  ASN.1 DER, encode an integer, Tom St Denis
*/


#ifdef LTC_DER

/**
  Store a short integer in the range (0,2^32-1)
  @param num      The integer to encode
  @param out      [out] The destination for the DER encoded integers
  @param outlen   [in/out] The max size and resulting size of the DER encoded integers
  @return CRYPT_OK if successful
*/
int der_encode_short_integer(unsigned long num, unsigned char *out, unsigned long *outlen)
{  
   unsigned long len, x, y, z;
   int           err;
   
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* force to 32 bits */
   num &= 0xFFFFFFFFUL;

   /* find out how big this will be */
   if ((err = der_length_short_integer(num, &len)) != CRYPT_OK) {
      return err;
   }

   if (*outlen < len) {
      *outlen = len;
      return CRYPT_BUFFER_OVERFLOW;
   }

   /* get len of output */
   z = 0;
   y = num;
   while (y) {
     ++z;
     y >>= 8;
   }

   /* handle zero */
   if (z == 0) {
      z = 1;
   }

   /* see if msb is set */
   z += (num&(1UL<<((z<<3) - 1))) ? 1 : 0;

   /* adjust the number so the msB is non-zero */
   for (x = 0; (z <= 4) && (x < (4 - z)); x++) {
      num <<= 8;
   }

   /* store header */
   x = 0;
   out[x++] = 0x02;
   out[x++] = (unsigned char)z;

   /* if 31st bit is set output a leading zero and decrement count */
   if (z == 5) {
      out[x++] = 0;
      --z;
   }

   /* store values */
   for (y = 0; y < z; y++) {
      out[x++] = (unsigned char)((num >> 24) & 0xFF);
      num    <<= 8;
   }

   /* we good */
   *outlen = x;
 
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/short_integer/der_encode_short_integer.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2006/12/28 01:27:24 $ */
