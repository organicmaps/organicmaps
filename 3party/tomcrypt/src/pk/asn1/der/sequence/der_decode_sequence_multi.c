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
#include <stdarg.h>


/**
  @file der_decode_sequence_multi.c
  ASN.1 DER, decode a SEQUENCE, Tom St Denis
*/

#ifdef LTC_DER

/**
  Decode a SEQUENCE type using a VA list
  @param in    Input buffer
  @param inlen Length of input in octets
  @remark <...> is of the form <type, size, data> (int, unsigned long, void*)
  @return CRYPT_OK on success
*/  
int der_decode_sequence_multi(const unsigned char *in, unsigned long inlen, ...)
{
   int           err, type;
   unsigned long size, x;
   void          *data;
   va_list       args;
   ltc_asn1_list *list;

   LTC_ARGCHK(in    != NULL);

   /* get size of output that will be required */
   va_start(args, inlen);
   x = 0;
   for (;;) {
       type = va_arg(args, int);
       size = va_arg(args, unsigned long);
       data = va_arg(args, void*);

       if (type == LTC_ASN1_EOL) { 
          break;
       }

       switch (type) {
           case LTC_ASN1_BOOLEAN:
           case LTC_ASN1_INTEGER:
           case LTC_ASN1_SHORT_INTEGER:
           case LTC_ASN1_BIT_STRING:
           case LTC_ASN1_OCTET_STRING:
           case LTC_ASN1_NULL:
           case LTC_ASN1_OBJECT_IDENTIFIER:
           case LTC_ASN1_IA5_STRING:
           case LTC_ASN1_PRINTABLE_STRING:
           case LTC_ASN1_UTF8_STRING:
           case LTC_ASN1_UTCTIME:
           case LTC_ASN1_SET:
           case LTC_ASN1_SETOF:
           case LTC_ASN1_SEQUENCE:
           case LTC_ASN1_CHOICE:
                ++x; 
                break;
          
           default:
               va_end(args);
               return CRYPT_INVALID_ARG;
       }
   }
   va_end(args);

   /* allocate structure for x elements */
   if (x == 0) {
      return CRYPT_NOP;
   }

   list = XCALLOC(sizeof(*list), x);
   if (list == NULL) {
      return CRYPT_MEM;
   }

   /* fill in the structure */
   va_start(args, inlen);
   x = 0;
   for (;;) {
       type = va_arg(args, int);
       size = va_arg(args, unsigned long);
       data = va_arg(args, void*);

       if (type == LTC_ASN1_EOL) { 
          break;
       }

       switch (type) {
           case LTC_ASN1_BOOLEAN:
           case LTC_ASN1_INTEGER:
           case LTC_ASN1_SHORT_INTEGER:
           case LTC_ASN1_BIT_STRING:
           case LTC_ASN1_OCTET_STRING:
           case LTC_ASN1_NULL:
           case LTC_ASN1_OBJECT_IDENTIFIER:
           case LTC_ASN1_IA5_STRING:
           case LTC_ASN1_PRINTABLE_STRING:
           case LTC_ASN1_UTF8_STRING:
           case LTC_ASN1_UTCTIME:
           case LTC_ASN1_SEQUENCE:
           case LTC_ASN1_SET:
           case LTC_ASN1_SETOF:          
           case LTC_ASN1_CHOICE:
                list[x].type   = type;
                list[x].size   = size;
                list[x++].data = data;
                break;
         
           default:
               va_end(args);
               err = CRYPT_INVALID_ARG;
               goto LBL_ERR;
       }
   }
   va_end(args);

   err = der_decode_sequence(in, inlen, list, x);
LBL_ERR:
   XFREE(list);
   return err;
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_multi.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2006/12/28 01:27:24 $ */
