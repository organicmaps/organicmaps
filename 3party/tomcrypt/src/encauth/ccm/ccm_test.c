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
  @file ccm_test.c
  CCM support, process a block of memory, Tom St Denis
*/

#ifdef LTC_CCM_MODE

int ccm_test(void)
{
#ifndef LTC_TEST
   return CRYPT_NOP;
#else
   static const struct {
       unsigned char key[16];
       unsigned char nonce[16];
       int           noncelen;
       unsigned char header[64];
       int           headerlen;
       unsigned char pt[64];
       int           ptlen;
       unsigned char ct[64];
       unsigned char tag[16];
       unsigned long taglen;
   } tests[] = {

/* 13 byte nonce, 8 byte auth, 23 byte pt */
{
   { 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
     0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF },
   { 0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xA0,
     0xA1, 0xA2, 0xA3, 0xA4, 0xA5 },
   13,
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 },
   8,
   { 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
     0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
     0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E },
   23,
   { 0x58, 0x8C, 0x97, 0x9A, 0x61, 0xC6, 0x63, 0xD2,
     0xF0, 0x66, 0xD0, 0xC2, 0xC0, 0xF9, 0x89, 0x80,
     0x6D, 0x5F, 0x6B, 0x61, 0xDA, 0xC3, 0x84 },
   { 0x17, 0xe8, 0xd1, 0x2c, 0xfd, 0xf9, 0x26, 0xe0 },
   8
},

/* 13 byte nonce, 12 byte header, 19 byte pt */
{
   { 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
     0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF },
   { 0x00, 0x00, 0x00, 0x06, 0x05, 0x04, 0x03, 0xA0,
     0xA1, 0xA2, 0xA3, 0xA4, 0xA5 },
   13,
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0A, 0x0B },
   12,
   { 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
     0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
     0x1C, 0x1D, 0x1E },
   19,
   { 0xA2, 0x8C, 0x68, 0x65, 0x93, 0x9A, 0x9A, 0x79,
     0xFA, 0xAA, 0x5C, 0x4C, 0x2A, 0x9D, 0x4A, 0x91,
     0xCD, 0xAC, 0x8C },
   { 0x96, 0xC8, 0x61, 0xB9, 0xC9, 0xE6, 0x1E, 0xF1 },
   8
},

/* supplied by Brian Gladman */
{
   { 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
     0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f },
   { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16  },
   7,
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 },
   8,
   { 0x20, 0x21, 0x22, 0x23 },
   4,
   { 0x71, 0x62, 0x01, 0x5b },
   { 0x4d, 0xac, 0x25, 0x5d },
   4
},

{
   { 0xc9, 0x7c, 0x1f, 0x67, 0xce, 0x37, 0x11, 0x85,
     0x51, 0x4a, 0x8a, 0x19, 0xf2, 0xbd, 0xd5, 0x2f },
   { 0x00, 0x50, 0x30, 0xf1, 0x84, 0x44, 0x08, 0xb5,
     0x03, 0x97, 0x76, 0xe7, 0x0c },
   13,
   { 0x08, 0x40, 0x0f, 0xd2, 0xe1, 0x28, 0xa5, 0x7c,
     0x50, 0x30, 0xf1, 0x84, 0x44, 0x08, 0xab, 0xae,
     0xa5, 0xb8, 0xfc, 0xba, 0x00, 0x00 },
   22,
   { 0xf8, 0xba, 0x1a, 0x55, 0xd0, 0x2f, 0x85, 0xae,
     0x96, 0x7b, 0xb6, 0x2f, 0xb6, 0xcd, 0xa8, 0xeb,
     0x7e, 0x78, 0xa0, 0x50 },
   20,
   { 0xf3, 0xd0, 0xa2, 0xfe, 0x9a, 0x3d, 0xbf, 0x23,
     0x42, 0xa6, 0x43, 0xe4, 0x32, 0x46, 0xe8, 0x0c,
     0x3c, 0x04, 0xd0, 0x19 },
   { 0x78, 0x45, 0xce, 0x0b, 0x16, 0xf9, 0x76, 0x23 },
   8
},

};
  unsigned long taglen, x, y;
  unsigned char buf[64], buf2[64], tag[16], tag2[16], tag3[16], zero[64];
  int           err, idx;
  symmetric_key skey;
  ccm_state ccm;
  
  zeromem(zero, 64);

  idx = find_cipher("aes");
  if (idx == -1) {
     idx = find_cipher("rijndael");
     if (idx == -1) {
        return CRYPT_NOP;
     }
  }

  for (x = 0; x < (sizeof(tests)/sizeof(tests[0])); x++) {
    for (y = 0; y < 2; y++) {
      taglen = tests[x].taglen;
      if (y == 0) {
         if ((err = cipher_descriptor[idx].setup(tests[x].key, 16, 0, &skey)) != CRYPT_OK) {
            return err;
         }

         if ((err = ccm_memory(idx,
                               tests[x].key, 16,
                               &skey,
                               tests[x].nonce, tests[x].noncelen,
                               tests[x].header, tests[x].headerlen,
                               (unsigned char*)tests[x].pt, tests[x].ptlen,
                               buf,
                               tag, &taglen, 0)) != CRYPT_OK) {
            return err;
         }
      } else {
         if ((err = ccm_init(&ccm, idx, tests[x].key, 16, tests[x].ptlen, tests[x].taglen, tests[x].headerlen)) != CRYPT_OK) {
            return err;
         }
         if ((err = ccm_add_nonce(&ccm, tests[x].nonce, tests[x].noncelen)) != CRYPT_OK) {
            return err;
         }
         if ((err = ccm_add_aad(&ccm, tests[x].header, tests[x].headerlen)) != CRYPT_OK) {
            return err;
         }
         if ((err = ccm_process(&ccm, (unsigned char*)tests[x].pt, tests[x].ptlen, buf, CCM_ENCRYPT)) != CRYPT_OK) {
            return err;
         }
         if ((err = ccm_done(&ccm, tag, &taglen)) != CRYPT_OK) {
            return err;
         }
      }

      if (XMEMCMP(buf, tests[x].ct, tests[x].ptlen)) {
#if defined(LTC_TEST_DBG)
         printf("\n%d: x=%lu y=%lu\n", __LINE__, x, y);
         print_hex("ct is    ", buf, tests[x].ptlen);
         print_hex("ct should", tests[x].ct, tests[x].ptlen);
#endif
         return CRYPT_FAIL_TESTVECTOR;
      }
      if (tests[x].taglen != taglen) {
#if defined(LTC_TEST_DBG)
         printf("\n%d: x=%lu y=%lu\n", __LINE__, x, y);
         printf("taglen %lu (is) %lu (should)\n", taglen, tests[x].taglen);
#endif
         return CRYPT_FAIL_TESTVECTOR;
      }
      if (XMEMCMP(tag, tests[x].tag, tests[x].taglen)) {
#if defined(LTC_TEST_DBG)
         printf("\n%d: x=%lu y=%lu\n", __LINE__, x, y);
         print_hex("tag is    ", tag, tests[x].taglen);
         print_hex("tag should", tests[x].tag, tests[x].taglen);
#endif
         return CRYPT_FAIL_TESTVECTOR;
      }

      if (y == 0) {
          XMEMCPY(tag3, tests[x].tag, tests[x].taglen);
          taglen = tests[x].taglen;
          if ((err = ccm_memory(idx,
                               tests[x].key, 16,
                               NULL,
                               tests[x].nonce, tests[x].noncelen,
                               tests[x].header, tests[x].headerlen,
                               buf2, tests[x].ptlen,
                               buf,
                               tag3, &taglen, 1   )) != CRYPT_OK) {
            return err;
         }
      } else {
         if ((err = ccm_init(&ccm, idx, tests[x].key, 16, tests[x].ptlen, tests[x].taglen, tests[x].headerlen)) != CRYPT_OK) {
            return err;
         }
         if ((err = ccm_add_nonce(&ccm, tests[x].nonce, tests[x].noncelen)) != CRYPT_OK) {
            return err;
         }
         if ((err = ccm_add_aad(&ccm, tests[x].header, tests[x].headerlen)) != CRYPT_OK) {
            return err;
         }
         if ((err = ccm_process(&ccm, buf2, tests[x].ptlen, buf, CCM_DECRYPT)) != CRYPT_OK) {
            return err;
         }
         if ((err = ccm_done(&ccm, tag2, &taglen)) != CRYPT_OK) {
            return err;
         }
      }

      if (XMEMCMP(buf2, tests[x].pt, tests[x].ptlen)) {
#if defined(LTC_TEST_DBG)
         printf("\n%d: x=%lu y=%lu\n", __LINE__, x, y);
         print_hex("pt is    ", buf2, tests[x].ptlen);
         print_hex("pt should", tests[x].pt, tests[x].ptlen);
#endif
         return CRYPT_FAIL_TESTVECTOR;
      }
      if (y == 0) {
        /* check if decryption with the wrong tag does not reveal the plaintext */
        XMEMCPY(tag3, tests[x].tag, tests[x].taglen);
        tag3[0] ^= 0xff; /* set the tag to the wrong value */
        taglen = tests[x].taglen;
        if ((err = ccm_memory(idx,
                              tests[x].key, 16,
                              NULL,
                              tests[x].nonce, tests[x].noncelen,
                              tests[x].header, tests[x].headerlen,
                              buf2, tests[x].ptlen,
                              buf,
                              tag3, &taglen, 1   )) != CRYPT_ERROR) {
          return CRYPT_FAIL_TESTVECTOR;
        }
        if (XMEMCMP(buf2, zero, tests[x].ptlen)) {
#if defined(LTC_CCM_TEST_DBG)
          printf("\n%d: x=%lu y=%lu\n", __LINE__, x, y);
          print_hex("pt is    ", buf2, tests[x].ptlen);
          print_hex("pt should", zero, tests[x].ptlen);
#endif
          return CRYPT_FAIL_TESTVECTOR;
        }
      } else {
        /* FIXME: Only check the tag if ccm_memory was not called: ccm_memory already
           validates the tag. ccm_process and ccm_done should somehow do the same,
           although with current setup it is impossible to keep the plaintext hidden
           if the tag is incorrect.
        */
        if (XMEMCMP(tag2, tests[x].tag, tests[x].taglen)) {
#if defined(LTC_TEST_DBG)
          printf("\n%d: x=%lu y=%lu\n", __LINE__, x, y);
          print_hex("tag is    ", tag2, tests[x].taglen);
          print_hex("tag should", tests[x].tag, tests[x].taglen);
#endif
          return CRYPT_FAIL_TESTVECTOR;
        }
      }

      if (y == 0) {
         cipher_descriptor[idx].done(&skey);
      }
    }
  }

  return CRYPT_OK;
#endif
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
