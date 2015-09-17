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
  @file der_length_printable_string.c
  ASN.1 DER, get length of Printable STRING, Tom St Denis
*/

#ifdef LTC_DER

static const struct {
   int code, value;
} printable_table[] = {
{ ' ', 32 }, 
{ '\'', 39 }, 
{ '(', 40 }, 
{ ')', 41 }, 
{ '+', 43 }, 
{ ',', 44 }, 
{ '-', 45 }, 
{ '.', 46 }, 
{ '/', 47 }, 
{ '0', 48 }, 
{ '1', 49 }, 
{ '2', 50 }, 
{ '3', 51 }, 
{ '4', 52 }, 
{ '5', 53 }, 
{ '6', 54 }, 
{ '7', 55 }, 
{ '8', 56 }, 
{ '9', 57 }, 
{ ':', 58 }, 
{ '=', 61 }, 
{ '?', 63 }, 
{ 'A', 65 }, 
{ 'B', 66 }, 
{ 'C', 67 }, 
{ 'D', 68 }, 
{ 'E', 69 }, 
{ 'F', 70 }, 
{ 'G', 71 }, 
{ 'H', 72 }, 
{ 'I', 73 }, 
{ 'J', 74 }, 
{ 'K', 75 }, 
{ 'L', 76 }, 
{ 'M', 77 }, 
{ 'N', 78 }, 
{ 'O', 79 }, 
{ 'P', 80 }, 
{ 'Q', 81 }, 
{ 'R', 82 }, 
{ 'S', 83 }, 
{ 'T', 84 }, 
{ 'U', 85 }, 
{ 'V', 86 }, 
{ 'W', 87 }, 
{ 'X', 88 }, 
{ 'Y', 89 }, 
{ 'Z', 90 }, 
{ 'a', 97 }, 
{ 'b', 98 }, 
{ 'c', 99 }, 
{ 'd', 100 }, 
{ 'e', 101 }, 
{ 'f', 102 }, 
{ 'g', 103 }, 
{ 'h', 104 }, 
{ 'i', 105 }, 
{ 'j', 106 }, 
{ 'k', 107 }, 
{ 'l', 108 }, 
{ 'm', 109 }, 
{ 'n', 110 }, 
{ 'o', 111 }, 
{ 'p', 112 }, 
{ 'q', 113 }, 
{ 'r', 114 }, 
{ 's', 115 }, 
{ 't', 116 }, 
{ 'u', 117 }, 
{ 'v', 118 }, 
{ 'w', 119 }, 
{ 'x', 120 }, 
{ 'y', 121 }, 
{ 'z', 122 }, 
};

int der_printable_char_encode(int c)
{
   int x;
   for (x = 0; x < (int)(sizeof(printable_table)/sizeof(printable_table[0])); x++) {
       if (printable_table[x].code == c) {
          return printable_table[x].value;
       }
   }
   return -1;
}

int der_printable_value_decode(int v)
{
   int x;
   for (x = 0; x < (int)(sizeof(printable_table)/sizeof(printable_table[0])); x++) {
       if (printable_table[x].value == v) {
          return printable_table[x].code;
       }
   }
   return -1;
}
   
/**
  Gets length of DER encoding of Printable STRING 
  @param octets   The values you want to encode 
  @param noctets  The number of octets in the string to encode
  @param outlen   [out] The length of the DER encoding for the given string
  @return CRYPT_OK if successful
*/
int der_length_printable_string(const unsigned char *octets, unsigned long noctets, unsigned long *outlen)
{
   unsigned long x;

   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(octets != NULL);

   /* scan string for validity */
   for (x = 0; x < noctets; x++) {
       if (der_printable_char_encode(octets[x]) == -1) {
          return CRYPT_INVALID_ARG;
       }
   }

   if (noctets < 128) {
      /* 16 LL DD DD DD ... */
      *outlen = 2 + noctets;
   } else if (noctets < 256) {
      /* 16 81 LL DD DD DD ... */
      *outlen = 3 + noctets;
   } else if (noctets < 65536UL) {
      /* 16 82 LL LL DD DD DD ... */
      *outlen = 4 + noctets;
   } else if (noctets < 16777216UL) {
      /* 16 83 LL LL LL DD DD DD ... */
      *outlen = 5 + noctets;
   } else {
      return CRYPT_INVALID_ARG;
   }

   return CRYPT_OK;
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */
