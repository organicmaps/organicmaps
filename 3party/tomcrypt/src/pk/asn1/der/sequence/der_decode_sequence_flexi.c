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

static unsigned long fetch_length(const unsigned char *in, unsigned long inlen, unsigned long *data_offset)
{
   unsigned long x, z;

   *data_offset = 0;

   /* skip type and read len */
   if (inlen < 2) {
      return 0xFFFFFFFF;
   }
   ++in; ++(*data_offset);

   /* read len */
   x = *in++; ++(*data_offset);

   /* <128 means literal */
   if (x < 128) {
      return x+*data_offset;
   }
   x     &= 0x7F; /* the lower 7 bits are the length of the length */
   inlen -= 2;

   /* len means len of len! */
   if (x == 0 || x > 4 || x > inlen) {
      return 0xFFFFFFFF;
   }

   *data_offset += x;
   z = 0;
   while (x--) {
      z = (z<<8) | ((unsigned long)*in);
      ++in;
   }
   return z+*data_offset;
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
   unsigned long err, type, len, totlen, data_offset;
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
      len = fetch_length(in, *inlen, &data_offset);
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

      if ((type & 0x20) && (type != 0x30) && (type != 0x31)) {
         /* constructed, use the 'used' field to store the original identifier */
         l->used = type;
         /* treat constructed elements like SETs */
         type = 0x20;
      }
      else if ((type & 0xC0) == 0x80) {
         /* context-specific, use the 'used' field to store the original identifier */
         l->used = type;
         /* context-specific elements are treated as opaque data */
         type = 0x80;
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

         case 0x14: /* TELETEXT */

            /* init field */
            l->type = LTC_ASN1_TELETEX_STRING;
            l->size = len;

            if ((l->data = XCALLOC(1, l->size)) == NULL) {
               err = CRYPT_MEM;
               goto error;
            }

            if ((err = der_decode_teletex_string(in, *inlen, l->data, &l->size)) != CRYPT_OK) {
               goto error;
            }

            if ((err = der_length_teletex_string(l->data, l->size, &len)) != CRYPT_OK) {
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

         case 0x20: /* Any CONSTRUCTED element that is neither SEQUENCE nor SET */
         case 0x30: /* SEQUENCE */
         case 0x31: /* SET */

             /* init field */
             if (type == 0x20) {
                l->type = LTC_ASN1_CONSTRUCTED;
             }
             else if (type == 0x30) {
                l->type = LTC_ASN1_SEQUENCE;
             }
             else {
                l->type = LTC_ASN1_SET;
             }

             /* jump to the start of the data */
             in     += data_offset;
             *inlen -= data_offset;
             len = len - data_offset;

             /* Sequence elements go as child */
             if ((err = der_decode_sequence_flexi(in, &len, &(l->child))) != CRYPT_OK) {
                goto error;
             }

             /* len update */
             totlen += data_offset;

             /* the flexi decoder can also do nothing, so make sure a child has been allocated */
             if (l->child) {
                /* link them up y0 */
                l->child->parent = l;
             }

             break;

         case 0x80: /* Context-specific */
             l->type = LTC_ASN1_CONTEXT_SPECIFIC;

             if ((l->data = XCALLOC(1, len - data_offset)) == NULL) {
                err = CRYPT_MEM;
                goto error;
             }

             XMEMCPY(l->data, in + data_offset, len - data_offset);
             l->size = len - data_offset;

             break;

         default:
           /* invalid byte ... this is a soft error */
           /* remove link */
           if (l->prev) {
              l       = l->prev;
              XFREE(l->next);
              l->next = NULL;
           }
           goto outside;
      }

      /* advance pointers */
      totlen  += len;
      in      += len;
      *inlen  -= len;
   }

outside:

   /* in case we processed anything */
   if (totlen) {
      /* rewind l please */
      while (l->prev != NULL || l->parent != NULL) {
         if (l->parent != NULL) {
            l = l->parent;
         } else {
            l = l->prev;
         }
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


/* $Source$ */
/* $Revision$ */
/* $Date$ */
