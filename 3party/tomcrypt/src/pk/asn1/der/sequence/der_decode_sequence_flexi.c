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
  @file der_decode_sequence_flexi.c
  ASN.1 DER, decode an array of ASN.1 types with a flexi parser, Tom St Denis
*/

#ifdef LTC_DER

static unsigned long fetch_length(const unsigned char *in, unsigned long inlen)
{
   unsigned long x, y, z;

   y = 0;

   /* skip type and read len */
   if (inlen < 2) {
      return 0xFFFFFFFF;
   }
   ++in; ++y;
   
   /* read len */
   x = *in++; ++y;
   
   /* <128 means literal */
   if (x < 128) {
      return x+y;
   }
   x     &= 0x7F; /* the lower 7 bits are the length of the length */
   inlen -= 2;
   
   /* len means len of len! */
   if (x == 0 || x > 4 || x > inlen) {
      return 0xFFFFFFFF;
   }
   
   y += x;
   z = 0;
   while (x--) {   
      z = (z<<8) | ((unsigned long)*in);
      ++in;
   }
   return z+y;
}

/** 
   ASN.1 DER Flexi(ble) decoder will decode arbitrary DER packets and create a linked list of the decoded elements.
   @param in      The input buffer
   @param inlen   [in/out] The length of the input buffer and on output the amount of decoded data 
   @param out     [out] A pointer to the linked list
   @return CRYPT_OK on success.
*/   
int der_decode_sequence_flexi(const unsigned char *in, unsigned long *inlen, ltc_asn1_list **out)
{
   ltc_asn1_list *l;
   unsigned long err, type, len, totlen, x, y;
   void          *realloc_tmp;
   
   LTC_ARGCHK(in    != NULL);
   LTC_ARGCHK(inlen != NULL);
   LTC_ARGCHK(out   != NULL);

   l = NULL;
   totlen = 0;
   
   /* scan the input and and get lengths and what not */
   while (*inlen) {     
      /* read the type byte */
      type = *in;

      /* fetch length */
      len = fetch_length(in, *inlen);
      if (len > *inlen) {
         err = CRYPT_INVALID_PACKET;
         goto error;
      }

      /* alloc new link */
      if (l == NULL) {
         l = XCALLOC(1, sizeof(*l));
         if (l == NULL) {
            err = CRYPT_MEM;
            goto error;
         }
      } else {
         l->next = XCALLOC(1, sizeof(*l));
         if (l->next == NULL) {
            err = CRYPT_MEM;
            goto error;
         }
         l->next->prev = l;
         l = l->next;
      }

      /* now switch on type */
      switch (type) {
         case 0x01: /* BOOLEAN */
            l->type = LTC_ASN1_BOOLEAN;
            l->size = 1;
            l->data = XCALLOC(1, sizeof(int));
       
            if ((err = der_decode_boolean(in, *inlen, l->data)) != CRYPT_OK) {
               goto error;
            }
        
            if ((err = der_length_boolean(&len)) != CRYPT_OK) {
               goto error;
            }
            break;

         case 0x02: /* INTEGER */
             /* init field */
             l->type = LTC_ASN1_INTEGER;
             l->size = 1;
             if ((err = mp_init(&l->data)) != CRYPT_OK) {
                 goto error;
             }
             
             /* decode field */
             if ((err = der_decode_integer(in, *inlen, l->data)) != CRYPT_OK) {
                 goto error;
             }
             
             /* calc length of object */
             if ((err = der_length_integer(l->data, &len)) != CRYPT_OK) {
                 goto error;
             }
             break;

         case 0x03: /* BIT */
            /* init field */
            l->type = LTC_ASN1_BIT_STRING;
            l->size = len * 8; /* *8 because we store decoded bits one per char and they are encoded 8 per char.  */

            if ((l->data = XCALLOC(1, l->size)) == NULL) {
               err = CRYPT_MEM;
               goto error;
            }
            
            if ((err = der_decode_bit_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
               goto error;
            }
            
            if ((err = der_length_bit_string(l->size, &len)) != CRYPT_OK) {
               goto error;
            }
            break;

         case 0x04: /* OCTET */

            /* init field */
            l->type = LTC_ASN1_OCTET_STRING;
            l->size = len;

            if ((l->data = XCALLOC(1, l->size)) == NULL) {
               err = CRYPT_MEM;
               goto error;
            }
            
            if ((err = der_decode_octet_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
               goto error;
            }
            
            if ((err = der_length_octet_string(l->size, &len)) != CRYPT_OK) {
               goto error;
            }
            break;

         case 0x05: /* NULL */
         
            /* valid NULL is 0x05 0x00 */
            if (in[0] != 0x05 || in[1] != 0x00) {
               err = CRYPT_INVALID_PACKET;
               goto error;
            }
            
            /* simple to store ;-) */
            l->type = LTC_ASN1_NULL;
            l->data = NULL;
            l->size = 0;
            len     = 2;
            
            break;
         
         case 0x06: /* OID */
         
            /* init field */
            l->type = LTC_ASN1_OBJECT_IDENTIFIER;
            l->size = len;

            if ((l->data = XCALLOC(len, sizeof(unsigned long))) == NULL) {
               err = CRYPT_MEM;
               goto error;
            }
            
            if ((err = der_decode_object_identifier(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
               goto error;
            }
            
            if ((err = der_length_object_identifier(l->data, l->size, &len)) != CRYPT_OK) {
               goto error;
            }
            
            /* resize it to save a bunch of mem */
            if ((realloc_tmp = XREALLOC(l->data, l->size * sizeof(unsigned long))) == NULL) {
               /* out of heap but this is not an error */
               break;
            }
            l->data = realloc_tmp;
            break;
  
         case 0x0C: /* UTF8 */
         
            /* init field */
            l->type = LTC_ASN1_UTF8_STRING;
            l->size = len;

            if ((l->data = XCALLOC(sizeof(wchar_t), l->size)) == NULL) {
               err = CRYPT_MEM;
               goto error;
            }
            
            if ((err = der_decode_utf8_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
               goto error;
            }
            
            if ((err = der_length_utf8_string(l->data, l->size, &len)) != CRYPT_OK) {
               goto error;
            }
            break;

         case 0x13: /* PRINTABLE */
         
            /* init field */
            l->type = LTC_ASN1_PRINTABLE_STRING;
            l->size = len;

            if ((l->data = XCALLOC(1, l->size)) == NULL) {
               err = CRYPT_MEM;
               goto error;
            }
            
            if ((err = der_decode_printable_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
               goto error;
            }
            
            if ((err = der_length_printable_string(l->data, l->size, &len)) != CRYPT_OK) {
               goto error;
            }
            break;
         
         case 0x16: /* IA5 */
         
            /* init field */
            l->type = LTC_ASN1_IA5_STRING;
            l->size = len;

            if ((l->data = XCALLOC(1, l->size)) == NULL) {
               err = CRYPT_MEM;
               goto error;
            }
            
            if ((err = der_decode_ia5_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
               goto error;
            }
            
            if ((err = der_length_ia5_string(l->data, l->size, &len)) != CRYPT_OK) {
               goto error;
            }
            break;
         
         case 0x17: /* UTC TIME */
         
            /* init field */
            l->type = LTC_ASN1_UTCTIME;
            l->size = 1;

            if ((l->data = XCALLOC(1, sizeof(ltc_utctime))) == NULL) {
               err = CRYPT_MEM;
               goto error;
            }
            
            len = *inlen;
            if ((err = der_decode_utctime(in, &len, l->data)) != CRYPT_OK) {
               goto error;
            }
            
            if ((err = der_length_utctime(l->data, &len)) != CRYPT_OK) {
               goto error;
            }
            break;
         
         case 0x30: /* SEQUENCE */
         case 0x31: /* SET */
         
             /* init field */
             l->type = (type == 0x30) ? LTC_ASN1_SEQUENCE : LTC_ASN1_SET;
             
             /* we have to decode the SEQUENCE header and get it's length */
             
                /* move past type */
                ++in; --(*inlen);
                
                /* read length byte */
                x = *in++; --(*inlen);
                
                /* smallest SEQUENCE/SET header */
                y = 2;
                
                /* now if it's > 127 the next bytes are the length of the length */
                if (x > 128) {
                   x      &= 0x7F;
                   in     += x;
                   *inlen -= x;
                   
                   /* update sequence header len */
                   y      += x;
                }
             
             /* Sequence elements go as child */
             len = len - y;
             if ((err = der_decode_sequence_flexi(in, &len, &(l->child))) != CRYPT_OK) {
                goto error;
             }
             
             /* len update */
             totlen += y;
             
             /* link them up y0 */
             l->child->parent = l;
             
             break;
         default:
           /* invalid byte ... this is a soft error */
           /* remove link */
           l       = l->prev;
           XFREE(l->next);
           l->next = NULL;
           goto outside;
      }
      
      /* advance pointers */
      totlen  += len;
      in      += len;
      *inlen  -= len;
   }
   
outside:   

   /* rewind l please */
   while (l->prev != NULL || l->parent != NULL) {
      if (l->parent != NULL) {
         l = l->parent;
      } else {
         l = l->prev;
      }
   }
   
   /* return */
   *out   = l;
   *inlen = totlen;
   return CRYPT_OK;

error:
   /* free list */
   der_sequence_free(l);

   return err;
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_flexi.c,v $ */
/* $Revision: 1.26 $ */
/* $Date: 2006/12/28 01:27:24 $ */
