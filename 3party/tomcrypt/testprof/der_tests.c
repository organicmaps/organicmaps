#include <tomcrypt_test.h>
#if defined(GMP_LTC_DESC) || defined(USE_GMP)
#include <gmp.h>
#endif

#ifndef LTC_DER

int der_tests(void)
{
   fprintf(stderr, "NOP");
   return 0;
}

#else

static void der_set_test(void)
{
   ltc_asn1_list list[10];
   static const unsigned char oct_str[] = { 1, 2, 3, 4 };
   static const unsigned char bin_str[] = { 1, 0, 0, 1 };
   static const unsigned long int_val   = 12345678UL;

   unsigned char strs[10][10], outbuf[128];
   unsigned long x, val, outlen;
   int           err;
   
   /* make structure and encode it */
   LTC_SET_ASN1(list, 0, LTC_ASN1_OCTET_STRING,  oct_str, sizeof(oct_str));
   LTC_SET_ASN1(list, 1, LTC_ASN1_BIT_STRING,    bin_str, sizeof(bin_str));
   LTC_SET_ASN1(list, 2, LTC_ASN1_SHORT_INTEGER, &int_val, 1);
   
   /* encode it */
   outlen = sizeof(outbuf);
   if ((err = der_encode_set(list, 3, outbuf, &outlen)) != CRYPT_OK) {
      fprintf(stderr, "error encoding set: %s\n", error_to_string(err));
      exit(EXIT_FAILURE);
   }
   
  
   /* first let's test the set_decoder out of order to see what happens, we should get all the fields we expect even though they're in a diff order */
   LTC_SET_ASN1(list, 0, LTC_ASN1_BIT_STRING,    strs[1], sizeof(strs[1]));
   LTC_SET_ASN1(list, 1, LTC_ASN1_SHORT_INTEGER, &val, 1);
   LTC_SET_ASN1(list, 2, LTC_ASN1_OCTET_STRING,  strs[0], sizeof(strs[0]));
   
   if ((err = der_decode_set(outbuf, outlen, list, 3)) != CRYPT_OK) {
      fprintf(stderr, "error decoding set using der_decode_set: %s\n", error_to_string(err));
      exit(EXIT_FAILURE);
   }
   
   /* now compare the items */
   if (memcmp(strs[0], oct_str, sizeof(oct_str))) {
      fprintf(stderr, "error decoding set using der_decode_set (oct_str is wrong):\n");
      exit(EXIT_FAILURE);
   }
      
   if (memcmp(strs[1], bin_str, sizeof(bin_str))) {
      fprintf(stderr, "error decoding set using der_decode_set (bin_str is wrong):\n");
      exit(EXIT_FAILURE);
   }
   
   if (val != int_val) {
      fprintf(stderr, "error decoding set using der_decode_set (int_val is wrong):\n");
      exit(EXIT_FAILURE);
   }
   
   strcpy((char*)strs[0], "one");
   strcpy((char*)strs[1], "one2");
   strcpy((char*)strs[2], "two");
   strcpy((char*)strs[3], "aaa");
   strcpy((char*)strs[4], "aaaa");
   strcpy((char*)strs[5], "aab");
   strcpy((char*)strs[6], "aaab");
   strcpy((char*)strs[7], "bbb");
   strcpy((char*)strs[8], "bbba");
   strcpy((char*)strs[9], "bbbb");
   
   for (x = 0; x < 10; x++) {
       LTC_SET_ASN1(list, x, LTC_ASN1_PRINTABLE_STRING, strs[x], strlen((char*)strs[x]));
   }
   
   outlen = sizeof(outbuf);
   if ((err = der_encode_setof(list, 10, outbuf, &outlen)) != CRYPT_OK) {       
      fprintf(stderr, "error encoding SET OF: %s\n", error_to_string(err));
      exit(EXIT_FAILURE);
   }
   
   for (x = 0; x < 10; x++) {
       LTC_SET_ASN1(list, x, LTC_ASN1_PRINTABLE_STRING, strs[x], sizeof(strs[x]) - 1);
   }
   XMEMSET(strs, 0, sizeof(strs));
   
   if ((err = der_decode_set(outbuf, outlen, list, 10)) != CRYPT_OK) {
      fprintf(stderr, "error decoding SET OF: %s\n", error_to_string(err));
      exit(EXIT_FAILURE);
   }
   
   /* now compare */
   for (x = 1; x < 10; x++) {
      if (!(strlen((char*)strs[x-1]) <= strlen((char*)strs[x])) && strcmp((char*)strs[x-1], (char*)strs[x]) >= 0) {
         fprintf(stderr, "error SET OF order at %lu is wrong\n", x);
         exit(EXIT_FAILURE);
      }
   }      
   
}


/* we are encoding 

  SEQUENCE {
     PRINTABLE "printable"
     IA5       "ia5"
     SEQUENCE {
        INTEGER 12345678
        UTCTIME { 91, 5, 6, 16, 45, 40, 1, 7, 0 }
        SEQUENCE {
           OCTET STRING { 1, 2, 3, 4 }
           BIT STRING   { 1, 0, 0, 1 }
           SEQUENCE {
              OID       { 1, 2, 840, 113549 }
              NULL
              SET OF {
                 PRINTABLE "333"  // WILL GET SORTED
                 PRINTABLE "222"
           }
        }
     }
  }     

*/  

static void der_flexi_test(void)
{
   static const char printable_str[]    = "printable";
   static const char set1_str[]         = "333";
   static const char set2_str[]         = "222";
   static const char ia5_str[]          = "ia5";
   static const unsigned long int_val   = 12345678UL;
   static const ltc_utctime   utctime   = { 91, 5, 6, 16, 45, 40, 1, 7, 0 };
   static const unsigned char oct_str[] = { 1, 2, 3, 4 };
   static const unsigned char bit_str[] = { 1, 0, 0, 1 };
   static const unsigned long oid_str[] = { 1, 2, 840, 113549 };
   
   unsigned char encode_buf[192];
   unsigned long encode_buf_len, decode_len;
   int           err;
   
   ltc_asn1_list static_list[5][3], *decoded_list, *l;
   
   /* build list */
   LTC_SET_ASN1(static_list[0], 0, LTC_ASN1_PRINTABLE_STRING, (void *)printable_str, strlen(printable_str));
   LTC_SET_ASN1(static_list[0], 1, LTC_ASN1_IA5_STRING,       (void *)ia5_str,       strlen(ia5_str));
   LTC_SET_ASN1(static_list[0], 2, LTC_ASN1_SEQUENCE,         static_list[1],   3);
   
   LTC_SET_ASN1(static_list[1], 0, LTC_ASN1_SHORT_INTEGER,    (void *)&int_val,         1);
   LTC_SET_ASN1(static_list[1], 1, LTC_ASN1_UTCTIME,          (void *)&utctime,         1);
   LTC_SET_ASN1(static_list[1], 2, LTC_ASN1_SEQUENCE,         static_list[2],   3);

   LTC_SET_ASN1(static_list[2], 0, LTC_ASN1_OCTET_STRING,     (void *)oct_str,          4);
   LTC_SET_ASN1(static_list[2], 1, LTC_ASN1_BIT_STRING,       (void *)bit_str,          4);
   LTC_SET_ASN1(static_list[2], 2, LTC_ASN1_SEQUENCE,         static_list[3],   3);

   LTC_SET_ASN1(static_list[3], 0, LTC_ASN1_OBJECT_IDENTIFIER,(void *)oid_str,          4);
   LTC_SET_ASN1(static_list[3], 1, LTC_ASN1_NULL,             NULL,             0);
   LTC_SET_ASN1(static_list[3], 2, LTC_ASN1_SETOF,            static_list[4],   2);

   LTC_SET_ASN1(static_list[4], 0, LTC_ASN1_PRINTABLE_STRING, set1_str, strlen(set1_str));
   LTC_SET_ASN1(static_list[4], 1, LTC_ASN1_PRINTABLE_STRING, set2_str, strlen(set2_str));

   /* encode it */
   encode_buf_len = sizeof(encode_buf);
   if ((err = der_encode_sequence(&static_list[0][0], 3, encode_buf, &encode_buf_len)) != CRYPT_OK) {
      fprintf(stderr, "Encoding static_list: %s\n", error_to_string(err));
      exit(EXIT_FAILURE);
   }
   
#if 0
   {
     FILE *f;
     f = fopen("t.bin", "wb");
     fwrite(encode_buf, 1, encode_buf_len, f);
     fclose(f);
   } 
#endif    
   
   /* decode with flexi */
   decode_len = encode_buf_len;
   if ((err = der_decode_sequence_flexi(encode_buf, &decode_len, &decoded_list)) != CRYPT_OK) {
      fprintf(stderr, "decoding static_list: %s\n", error_to_string(err));
      exit(EXIT_FAILURE);
   }
   
   if (decode_len != encode_buf_len) {
      fprintf(stderr, "Decode len of %lu does not match encode len of %lu \n", decode_len, encode_buf_len);
      exit(EXIT_FAILURE);
   }
   
   /* we expect l->next to be NULL and l->child to not be */
   l = decoded_list;
   if (l->next != NULL || l->child == NULL) {
      fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
      exit(EXIT_FAILURE);
   }
   
   /* we expect a SEQUENCE */
      if (l->type != LTC_ASN1_SEQUENCE) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      l = l->child;
         
   /* PRINTABLE STRING */
      /* we expect printable_str */
      if (l->next == NULL || l->child != NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (l->type != LTC_ASN1_PRINTABLE_STRING) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (l->size != strlen(printable_str) || memcmp(printable_str, l->data, l->size)) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      /* move to next */
      l = l->next;
      
   /* IA5 STRING */      
      /* we expect ia5_str */
      if (l->next == NULL || l->child != NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_IA5_STRING) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (l->size != strlen(ia5_str) || memcmp(ia5_str, l->data, l->size)) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      /* move to next */
      l = l->next;
   
   /* expect child anve move down */
      
      if (l->next != NULL || l->child == NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_SEQUENCE) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      l = l->child;
      

   /* INTEGER */
   
      if (l->next == NULL || l->child != NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_INTEGER) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (mp_cmp_d(l->data, 12345678UL) != LTC_MP_EQ) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      /* move to next */
      l = l->next;
      
   /* UTCTIME */
         
      if (l->next == NULL || l->child != NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_UTCTIME) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (memcmp(l->data, &utctime, sizeof(utctime))) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      /* move to next */
      l = l->next;
      
   /* expect child anve move down */
      
      if (l->next != NULL || l->child == NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_SEQUENCE) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      l = l->child;
      
      
   /* OCTET STRING */      
      /* we expect oct_str */
      if (l->next == NULL || l->child != NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_OCTET_STRING) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (l->size != sizeof(oct_str) || memcmp(oct_str, l->data, l->size)) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      /* move to next */
      l = l->next;

   /* BIT STRING */      
      /* we expect oct_str */
      if (l->next == NULL || l->child != NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_BIT_STRING) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (l->size != sizeof(bit_str) || memcmp(bit_str, l->data, l->size)) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      /* move to next */
      l = l->next;

   /* expect child anve move down */
      
      if (l->next != NULL || l->child == NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_SEQUENCE) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      l = l->child;


   /* OID STRING */      
      /* we expect oid_str */
      if (l->next == NULL || l->child != NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_OBJECT_IDENTIFIER) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (l->size != sizeof(oid_str)/sizeof(oid_str[0]) || memcmp(oid_str, l->data, l->size*sizeof(oid_str[0]))) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      /* move to next */
      l = l->next;
      
   /* NULL */
      if (l->type != LTC_ASN1_NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      /* move to next */
      l = l->next;
      
   /* expect child anve move down */
      if (l->next != NULL || l->child == NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      
      if (l->type != LTC_ASN1_SET) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
      l = l->child;
      
   /* PRINTABLE STRING */
      /* we expect printable_str */
      if (l->next == NULL || l->child != NULL) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (l->type != LTC_ASN1_PRINTABLE_STRING) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
/* note we compare set2_str FIRST because the SET OF is sorted and "222" comes before "333" */   
      if (l->size != strlen(set2_str) || memcmp(set2_str, l->data, l->size)) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      /* move to next */
      l = l->next;

   /* PRINTABLE STRING */
      /* we expect printable_str */
      if (l->type != LTC_ASN1_PRINTABLE_STRING) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   
      if (l->size != strlen(set1_str) || memcmp(set1_str, l->data, l->size)) {
         fprintf(stderr, "(%d), %d, %lu, next=%p, prev=%p, parent=%p, child=%p\n", __LINE__, l->type, l->size, l->next, l->prev, l->parent, l->child);
         exit(EXIT_FAILURE);
      }
   

   der_sequence_free(l);

}

static int der_choice_test(void)
{
   ltc_asn1_list types[7], host[1];
   unsigned char bitbuf[10], octetbuf[10], ia5buf[10], printbuf[10], outbuf[256];
   unsigned long integer, oidbuf[10], outlen, inlen, x, y;
   void          *mpinteger;
   ltc_utctime   utctime = { 91, 5, 6, 16, 45, 40, 1, 7, 0 };

   /* setup variables */
   for (x = 0; x < sizeof(bitbuf); x++)   { bitbuf[x]   = x & 1; }
   for (x = 0; x < sizeof(octetbuf); x++) { octetbuf[x] = x;     }
   for (x = 0; x < sizeof(ia5buf); x++)   { ia5buf[x]   = 'a';   }
   for (x = 0; x < sizeof(printbuf); x++) { printbuf[x] = 'a';   }
   integer = 1;
   for (x = 0; x < sizeof(oidbuf)/sizeof(oidbuf[0]); x++)   { oidbuf[x] = x + 1;   }
   DO(mp_init(&mpinteger));

   for (x = 0; x < 14; x++) {
       /* setup list */
       LTC_SET_ASN1(types, 0, LTC_ASN1_PRINTABLE_STRING, printbuf, sizeof(printbuf));
       LTC_SET_ASN1(types, 1, LTC_ASN1_BIT_STRING, bitbuf, sizeof(bitbuf));
       LTC_SET_ASN1(types, 2, LTC_ASN1_OCTET_STRING, octetbuf, sizeof(octetbuf));
       LTC_SET_ASN1(types, 3, LTC_ASN1_IA5_STRING, ia5buf, sizeof(ia5buf));
       if (x > 7) {
          LTC_SET_ASN1(types, 4, LTC_ASN1_SHORT_INTEGER, &integer, 1);
       } else {
          LTC_SET_ASN1(types, 4, LTC_ASN1_INTEGER, mpinteger, 1);
       }
       LTC_SET_ASN1(types, 5, LTC_ASN1_OBJECT_IDENTIFIER, oidbuf, sizeof(oidbuf)/sizeof(oidbuf[0]));
       LTC_SET_ASN1(types, 6, LTC_ASN1_UTCTIME, &utctime, 1);

       LTC_SET_ASN1(host, 0, LTC_ASN1_CHOICE, types, 7);

       
       /* encode */
       outlen = sizeof(outbuf);
       DO(der_encode_sequence(&types[x>6?x-7:x], 1, outbuf, &outlen));

       /* decode it */
       inlen = outlen;
       DO(der_decode_sequence(outbuf, inlen, &host[0], 1));

       for (y = 0; y < 7; y++) {
           if (types[y].used && y != (x>6?x-7:x)) {
               fprintf(stderr, "CHOICE, flag %lu in trial %lu was incorrectly set to one\n", y, x);
               return 1;
           }
           if (!types[y].used && y == (x>6?x-7:x)) {
               fprintf(stderr, "CHOICE, flag %lu in trial %lu was incorrectly set to zero\n", y, x);
               return 1;
           }
      }
  }
  mp_clear(mpinteger);
  return 0;
}
   

int der_tests(void)
{
   unsigned long x, y, z, zz, oid[2][32];
   unsigned char buf[3][2048];
   void *a, *b, *c, *d, *e, *f, *g;

   static const unsigned char rsa_oid_der[] = { 0x06, 0x06, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d };
   static const unsigned long rsa_oid[]     = { 1, 2, 840, 113549 };

   static const unsigned char rsa_ia5[]     = "test1@rsa.com";
   static const unsigned char rsa_ia5_der[] = { 0x16, 0x0d, 0x74, 0x65, 0x73, 0x74, 0x31,
                                                0x40, 0x72, 0x73, 0x61, 0x2e, 0x63, 0x6f, 0x6d };

   static const unsigned char rsa_printable[] = "Test User 1";
   static const unsigned char rsa_printable_der[] = { 0x13, 0x0b, 0x54, 0x65, 0x73, 0x74, 0x20, 0x55, 
                                                      0x73, 0x65, 0x72, 0x20, 0x31 };

   static const ltc_utctime   rsa_time1 = { 91, 5, 6, 16, 45, 40, 1, 7, 0 };
   static const ltc_utctime   rsa_time2 = { 91, 5, 6, 23, 45, 40, 0, 0, 0 };
   ltc_utctime                tmp_time;

   static const unsigned char rsa_time1_der[] = { 0x17, 0x11, 0x39, 0x31, 0x30, 0x35, 0x30, 0x36, 0x31, 0x36, 0x34, 0x35, 0x34, 0x30, 0x2D, 0x30, 0x37, 0x30, 0x30 };
   static const unsigned char rsa_time2_der[] = { 0x17, 0x0d, 0x39, 0x31, 0x30, 0x35, 0x30, 0x36, 0x32, 0x33, 0x34, 0x35, 0x34, 0x30, 0x5a };

   static const wchar_t utf8_1[]           = { 0x0041, 0x2262, 0x0391, 0x002E };
   static const unsigned char utf8_1_der[] = { 0x0C, 0x07, 0x41, 0xE2, 0x89, 0xA2, 0xCE, 0x91, 0x2E };
   static const wchar_t utf8_2[]           = { 0xD55C, 0xAD6D, 0xC5B4 };
   static const unsigned char utf8_2_der[] = { 0x0C, 0x09, 0xED, 0x95, 0x9C, 0xEA, 0xB5, 0xAD, 0xEC, 0x96, 0xB4 };

   unsigned char utf8_buf[32];
   wchar_t utf8_out[32];

   DO(mp_init_multi(&a, &b, &c, &d, &e, &f, &g, NULL));
   for (zz = 0; zz < 16; zz++) {
#ifdef USE_TFM
      for (z = 0; z < 256; z++) {
#else
      for (z = 0; z < 1024; z++) {
#endif
         if (yarrow_read(buf[0], z, &yarrow_prng) != z) {
            fprintf(stderr, "Failed to read %lu bytes from yarrow\n", z);
            return 1;
         }
         DO(mp_read_unsigned_bin(a, buf[0], z));
/*          if (mp_iszero(a) == LTC_MP_NO) { a.sign = buf[0][0] & 1 ? LTC_MP_ZPOS : LTC_MP_NEG; } */
         x = sizeof(buf[0]);
         DO(der_encode_integer(a, buf[0], &x));
         DO(der_length_integer(a, &y));
         if (y != x) { fprintf(stderr, "DER INTEGER size mismatch\n"); return 1; }
         mp_set_int(b, 0);
         DO(der_decode_integer(buf[0], y, b));
         if (y != x || mp_cmp(a, b) != LTC_MP_EQ) {
            fprintf(stderr, "%lu: %lu vs %lu\n", z, x, y);
            mp_clear_multi(a, b, c, d, e, f, g, NULL);
            return 1;
         }
      }
   }

/* test short integer */
   for (zz = 0; zz < 256; zz++) {
      for (z = 1; z < 4; z++) {
         if (yarrow_read(buf[0], z, &yarrow_prng) != z) {
            fprintf(stderr, "Failed to read %lu bytes from yarrow\n", z);
            return 1;
         }
         /* encode with normal */
         DO(mp_read_unsigned_bin(a, buf[0], z));

         x = sizeof(buf[0]);
         DO(der_encode_integer(a, buf[0], &x));

         /* encode with short */
         y = sizeof(buf[1]);
         DO(der_encode_short_integer(mp_get_int(a), buf[1], &y));
         if (x != y || memcmp(buf[0], buf[1], x)) {
            fprintf(stderr, "DER INTEGER short encoding failed, %lu, %lu\n", x, y);
            for (z = 0; z < x; z++) fprintf(stderr, "%02x ", buf[0][z]); fprintf(stderr, "\n");
            for (z = 0; z < y; z++) fprintf(stderr, "%02x ", buf[1][z]); fprintf(stderr, "\n");
            mp_clear_multi(a, b, c, d, e, f, g, NULL);
            return 1;
         }

         /* decode it */
         x = 0;
         DO(der_decode_short_integer(buf[1], y, &x));
         if (x != mp_get_int(a)) {
            fprintf(stderr, "DER INTEGER short decoding failed, %lu, %lu\n", x, mp_get_int(a));
            mp_clear_multi(a, b, c, d, e, f, g, NULL);
            return 1;
         }
      }
   } 
   mp_clear_multi(a, b, c, d, e, f, g, NULL);

   
/* Test bit string */
   for (zz = 1; zz < 1536; zz++) {
       yarrow_read(buf[0], zz, &yarrow_prng);
       for (z = 0; z < zz; z++) {
           buf[0][z] &= 0x01;
       }
       x = sizeof(buf[1]);
       DO(der_encode_bit_string(buf[0], zz, buf[1], &x));
       DO(der_length_bit_string(zz, &y));
       if (y != x) { 
          fprintf(stderr, "\nDER BIT STRING length of encoded not match expected : %lu, %lu, %lu\n", z, x, y);
          return 1;
       }

       y = sizeof(buf[2]);
       DO(der_decode_bit_string(buf[1], x, buf[2], &y));
       if (y != zz || memcmp(buf[0], buf[2], zz)) {
          fprintf(stderr, "%lu, %lu, %d\n", y, zz, memcmp(buf[0], buf[2], zz));
          return 1;
       }
   }

/* Test octet string */
   for (zz = 1; zz < 1536; zz++) {
       yarrow_read(buf[0], zz, &yarrow_prng);
       x = sizeof(buf[1]);
       DO(der_encode_octet_string(buf[0], zz, buf[1], &x));
       DO(der_length_octet_string(zz, &y));
       if (y != x) { 
          fprintf(stderr, "\nDER OCTET STRING length of encoded not match expected : %lu, %lu, %lu\n", z, x, y);
          return 1;
       }
       y = sizeof(buf[2]);
       DO(der_decode_octet_string(buf[1], x, buf[2], &y));
       if (y != zz || memcmp(buf[0], buf[2], zz)) {
          fprintf(stderr, "%lu, %lu, %d\n", y, zz, memcmp(buf[0], buf[2], zz));
          return 1;
       }
   }

/* test OID */
   x = sizeof(buf[0]);
   DO(der_encode_object_identifier((unsigned long*)rsa_oid, sizeof(rsa_oid)/sizeof(rsa_oid[0]), buf[0], &x));
   if (x != sizeof(rsa_oid_der) || memcmp(rsa_oid_der, buf[0], x)) {
      fprintf(stderr, "rsa_oid_der encode failed to match, %lu, ", x);
      for (y = 0; y < x; y++) fprintf(stderr, "%02x ", buf[0][y]);
      fprintf(stderr, "\n");
      return 1;
   }

   y = sizeof(oid[0])/sizeof(oid[0][0]);
   DO(der_decode_object_identifier(buf[0], x, oid[0], &y));
   if (y != sizeof(rsa_oid)/sizeof(rsa_oid[0]) || memcmp(rsa_oid, oid[0], sizeof(rsa_oid))) {
      fprintf(stderr, "rsa_oid_der decode failed to match, %lu, ", y);
      for (z = 0; z < y; z++) fprintf(stderr, "%lu ", oid[0][z]);
      fprintf(stderr, "\n");
      return 1;
   }

   /* do random strings */
   for (zz = 0; zz < 5000; zz++) {
       /* pick a random number of words */
       yarrow_read(buf[0], 4, &yarrow_prng);
       LOAD32L(z, buf[0]);
       z = 2 + (z % ((sizeof(oid[0])/sizeof(oid[0][0])) - 2));
       
       /* fill them in */
       oid[0][0] = buf[0][0] % 3;
       oid[0][1] = buf[0][1] % 40;

       for (y = 2; y < z; y++) {
          yarrow_read(buf[0], 4, &yarrow_prng);
          LOAD32L(oid[0][y], buf[0]);
       }

       /* encode it */
       x = sizeof(buf[0]);
       DO(der_encode_object_identifier(oid[0], z, buf[0], &x));
       DO(der_length_object_identifier(oid[0], z, &y));
       if (x != y) {
          fprintf(stderr, "Random OID %lu test failed, length mismatch: %lu, %lu\n", z, x, y);
          for (x = 0; x < z; x++) fprintf(stderr, "%lu\n", oid[0][x]);
          return 1;
       }
       
       /* decode it */
       y = sizeof(oid[0])/sizeof(oid[0][0]);
       DO(der_decode_object_identifier(buf[0], x, oid[1], &y));
       if (y != z) {
          fprintf(stderr, "Random OID %lu test failed, decode length mismatch: %lu, %lu\n", z, x, y);
          return 1;
       }
       if (memcmp(oid[0], oid[1], sizeof(oid[0][0]) * z)) {
          fprintf(stderr, "Random OID %lu test failed, decoded values wrong\n", z);
          for (x = 0; x < z; x++) fprintf(stderr, "%lu\n", oid[0][x]); fprintf(stderr, "\n\n Got \n\n");
          for (x = 0; x < z; x++) fprintf(stderr, "%lu\n", oid[1][x]);
          return 1;
       }
   }

/* IA5 string */
   x = sizeof(buf[0]);
   DO(der_encode_ia5_string(rsa_ia5, strlen((char*)rsa_ia5), buf[0], &x));
   if (x != sizeof(rsa_ia5_der) || memcmp(buf[0], rsa_ia5_der, x)) {
      fprintf(stderr, "IA5 encode failed: %lu, %lu\n", x, (unsigned long)sizeof(rsa_ia5_der));
      return 1;
   }
   DO(der_length_ia5_string(rsa_ia5, strlen((char*)rsa_ia5), &y));
   if (y != x) {
      fprintf(stderr, "IA5 length failed to match: %lu, %lu\n", x, y);
      return 1;
   }
   y = sizeof(buf[1]);
   DO(der_decode_ia5_string(buf[0], x, buf[1], &y));
   if (y != strlen((char*)rsa_ia5) || memcmp(buf[1], rsa_ia5, strlen((char*)rsa_ia5))) {
       fprintf(stderr, "DER IA5 failed test vector\n");
       return 1;
   }

/* Printable string */
   x = sizeof(buf[0]);
   DO(der_encode_printable_string(rsa_printable, strlen((char*)rsa_printable), buf[0], &x));
   if (x != sizeof(rsa_printable_der) || memcmp(buf[0], rsa_printable_der, x)) {
      fprintf(stderr, "PRINTABLE encode failed: %lu, %lu\n", x, (unsigned long)sizeof(rsa_printable_der));
      return 1;
   }
   DO(der_length_printable_string(rsa_printable, strlen((char*)rsa_printable), &y));
   if (y != x) {
      fprintf(stderr, "printable length failed to match: %lu, %lu\n", x, y);
      return 1;
   }
   y = sizeof(buf[1]);
   DO(der_decode_printable_string(buf[0], x, buf[1], &y));
   if (y != strlen((char*)rsa_printable) || memcmp(buf[1], rsa_printable, strlen((char*)rsa_printable))) {
       fprintf(stderr, "DER printable failed test vector\n");
       return 1;
   }

/* Test UTC time */
   x = sizeof(buf[0]);
   DO(der_encode_utctime((ltc_utctime*)&rsa_time1, buf[0], &x));
   if (x != sizeof(rsa_time1_der) || memcmp(buf[0], rsa_time1_der, x)) {
      fprintf(stderr, "UTCTIME encode of rsa_time1 failed: %lu, %lu\n", x, (unsigned long)sizeof(rsa_time1_der));
fprintf(stderr, "\n\n");
for (y = 0; y < x; y++) fprintf(stderr, "%02x ", buf[0][y]); printf("\n");

      return 1;
   }
   DO(der_length_utctime((ltc_utctime*)&rsa_time1, &y));
   if (y != x) {
      fprintf(stderr, "UTCTIME length failed to match for rsa_time1: %lu, %lu\n", x, y);
      return 1;
   }
   DO(der_decode_utctime(buf[0], &y, &tmp_time));
   if (y != x || memcmp(&rsa_time1, &tmp_time, sizeof(ltc_utctime))) {
      fprintf(stderr, "UTCTIME decode failed for rsa_time1: %lu %lu\n", x, y);
fprintf(stderr, "\n\n%u %u %u %u %u %u %u %u %u\n\n", 
tmp_time.YY,
tmp_time.MM,
tmp_time.DD,
tmp_time.hh,
tmp_time.mm,
tmp_time.ss,
tmp_time.off_dir,
tmp_time.off_mm,
tmp_time.off_hh);
      return 1;
   }

   x = sizeof(buf[0]);
   DO(der_encode_utctime((ltc_utctime*)&rsa_time2, buf[0], &x));
   if (x != sizeof(rsa_time2_der) || memcmp(buf[0], rsa_time2_der, x)) {
      fprintf(stderr, "UTCTIME encode of rsa_time2 failed: %lu, %lu\n", x, (unsigned long)sizeof(rsa_time1_der));
fprintf(stderr, "\n\n");
for (y = 0; y < x; y++) fprintf(stderr, "%02x ", buf[0][y]); printf("\n");

      return 1;
   }
   DO(der_length_utctime((ltc_utctime*)&rsa_time2, &y));
   if (y != x) {
      fprintf(stderr, "UTCTIME length failed to match for rsa_time2: %lu, %lu\n", x, y);
      return 1;
   }
   DO(der_decode_utctime(buf[0], &y, &tmp_time));
   if (y != x || memcmp(&rsa_time2, &tmp_time, sizeof(ltc_utctime))) {
      fprintf(stderr, "UTCTIME decode failed for rsa_time2: %lu %lu\n", x, y);
fprintf(stderr, "\n\n%u %u %u %u %u %u %u %u %u\n\n", 
tmp_time.YY,
tmp_time.MM,
tmp_time.DD,
tmp_time.hh,
tmp_time.mm,
tmp_time.ss,
tmp_time.off_dir,
tmp_time.off_mm,
tmp_time.off_hh);


      return 1;
   }

   /* UTF 8 */
     /* encode it */
     x = sizeof(utf8_buf);
     DO(der_encode_utf8_string(utf8_1, sizeof(utf8_1) / sizeof(utf8_1[0]), utf8_buf, &x));
     if (x != sizeof(utf8_1_der) || memcmp(utf8_buf, utf8_1_der, x)) {
        fprintf(stderr, "DER UTF8_1 encoded to %lu bytes\n", x);
        for (y = 0; y < x; y++) fprintf(stderr, "%02x ", (unsigned)utf8_buf[y]); fprintf(stderr, "\n");
        return 1;
     }
     /* decode it */
     y = sizeof(utf8_out) / sizeof(utf8_out[0]);
     DO(der_decode_utf8_string(utf8_buf, x, utf8_out, &y));
     if (y != (sizeof(utf8_1) / sizeof(utf8_1[0])) || memcmp(utf8_1, utf8_out, y * sizeof(wchar_t))) {
        fprintf(stderr, "DER UTF8_1 decoded to %lu wchar_t\n", y);
        for (x = 0; x < y; x++) fprintf(stderr, "%04lx ", (unsigned long)utf8_out[x]); fprintf(stderr, "\n");
        return 1;
     }

     /* encode it */
     x = sizeof(utf8_buf);
     DO(der_encode_utf8_string(utf8_2, sizeof(utf8_2) / sizeof(utf8_2[0]), utf8_buf, &x));
     if (x != sizeof(utf8_2_der) || memcmp(utf8_buf, utf8_2_der, x)) {
        fprintf(stderr, "DER UTF8_2 encoded to %lu bytes\n", x);
        for (y = 0; y < x; y++) fprintf(stderr, "%02x ", (unsigned)utf8_buf[y]); fprintf(stderr, "\n");
        return 1;
     }
     /* decode it */
     y = sizeof(utf8_out) / sizeof(utf8_out[0]);
     DO(der_decode_utf8_string(utf8_buf, x, utf8_out, &y));
     if (y != (sizeof(utf8_2) / sizeof(utf8_2[0])) || memcmp(utf8_2, utf8_out, y * sizeof(wchar_t))) {
        fprintf(stderr, "DER UTF8_2 decoded to %lu wchar_t\n", y);
        for (x = 0; x < y; x++) fprintf(stderr, "%04lx ", (unsigned long)utf8_out[x]); fprintf(stderr, "\n");
        return 1;
     }


   der_set_test();
   der_flexi_test();
   return der_choice_test();
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/testprof/der_tests.c,v $ */
/* $Revision: 1.50 $ */
/* $Date: 2007/05/12 14:20:27 $ */
