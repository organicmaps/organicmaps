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
  @file der_length_integer.c
  ASN.1 DER, get length of encoding, Tom St Denis
*/


#ifdef LTC_DER
/**
  Gets length of DER encoding of num 
  @param num    The int to get the size of 
  @param outlen [out] The length of the DER encoding for the given integer
  @return CRYPT_OK if successful
*/
int der_length_integer(void *num, unsigned long *outlen)
{
   unsigned long z, len;
   int           leading_zero;

   LTC_ARGCHK(num     != NULL);
   LTC_ARGCHK(outlen  != NULL);

   if (mp_cmp_d(num, 0) != LTC_MP_LT) {
      /* positive */

      /* we only need a leading zero if the msb of the first byte is one */
      if ((mp_count_bits(num) & 7) == 0 || mp_iszero(num) == LTC_MP_YES) {
         leading_zero = 1;
      } else {
         leading_zero = 0;
      }

      /* size for bignum */
      z = len = leading_zero + mp_unsigned_bin_size(num);
   } else {
      /* it's negative */
      /* find power of 2 that is a multiple of eight and greater than count bits */
      leading_zero = 0;
      z = mp_count_bits(num);
      z = z + (8 - (z & 7));
      if (((mp_cnt_lsb(num)+1)==mp_count_bits(num)) && ((mp_count_bits(num)&7)==0)) --z;
      len = z = z >> 3;
   }

   /* now we need a length */
   if (z < 128) {
      /* short form */
      ++len;
   } else {
      /* long form (relies on z != 0), assumes length bytes < 128 */
      ++len;

      while (z) {
         ++len;
         z >>= 8;
      }
   }

   /* we need a 0x02 to indicate it's INTEGER */
   ++len;

   /* return length */
   *outlen = len; 
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/integer/der_length_integer.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */
