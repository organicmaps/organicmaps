/* test CFB/OFB/CBC modes */
#include <tomcrypt_test.h>

int modes_test(void)
{
   unsigned char pt[64], ct[64], tmp[64], key[16], iv[16], iv2[16];
   int cipher_idx;
#ifdef LTC_CBC_MODE
   symmetric_CBC cbc;
#endif
#ifdef LTC_CFB_MODE
   symmetric_CFB cfb;
#endif
#ifdef LTC_OFB_MODE
   symmetric_OFB ofb;
#endif
   unsigned long l;
   
   /* make a random pt, key and iv */
   yarrow_read(pt,  64, &yarrow_prng);
   yarrow_read(key, 16, &yarrow_prng);
   yarrow_read(iv,  16, &yarrow_prng);
   
   /* get idx of AES handy */
   cipher_idx = find_cipher("aes");
   if (cipher_idx == -1) {
      fprintf(stderr, "test requires AES");
      return 1;
   }
   
#ifdef LTC_F8_MODE
   DO(f8_test_mode());
#endif   
   
#ifdef LTC_LRW_MODE
   DO(lrw_test());
#endif

#ifdef LTC_CBC_MODE
   /* test CBC mode */
   /* encode the block */
   DO(cbc_start(cipher_idx, iv, key, 16, 0, &cbc));
   l = sizeof(iv2);
   DO(cbc_getiv(iv2, &l, &cbc));
   if (l != 16 || memcmp(iv2, iv, 16)) {
      fprintf(stderr, "cbc_getiv failed");
      return 1;
   }
   DO(cbc_encrypt(pt, ct, 64, &cbc));
   
   /* decode the block */
   DO(cbc_setiv(iv2, l, &cbc));
   zeromem(tmp, sizeof(tmp));
   DO(cbc_decrypt(ct, tmp, 64, &cbc));
   if (memcmp(tmp, pt, 64) != 0) {
      fprintf(stderr, "CBC failed");
      return 1;
   }
#endif

#ifdef LTC_CFB_MODE
   /* test CFB mode */
   /* encode the block */
   DO(cfb_start(cipher_idx, iv, key, 16, 0, &cfb));
   l = sizeof(iv2);
   DO(cfb_getiv(iv2, &l, &cfb));
   /* note we don't memcmp iv2/iv since cfb_start processes the IV for the first block */
   if (l != 16) {
      fprintf(stderr, "cfb_getiv failed");
      return 1;
   }
   DO(cfb_encrypt(pt, ct, 64, &cfb));
   
   /* decode the block */
   DO(cfb_setiv(iv, l, &cfb));
   zeromem(tmp, sizeof(tmp));
   DO(cfb_decrypt(ct, tmp, 64, &cfb));
   if (memcmp(tmp, pt, 64) != 0) {
      fprintf(stderr, "CFB failed");
      return 1;
   }
#endif
   
#ifdef LTC_OFB_MODE
   /* test OFB mode */
   /* encode the block */
   DO(ofb_start(cipher_idx, iv, key, 16, 0, &ofb));
   l = sizeof(iv2);
   DO(ofb_getiv(iv2, &l, &ofb));
   if (l != 16 || memcmp(iv2, iv, 16)) {
      fprintf(stderr, "ofb_getiv failed");
      return 1;
   }
   DO(ofb_encrypt(pt, ct, 64, &ofb));
   
   /* decode the block */
   DO(ofb_setiv(iv2, l, &ofb));
   zeromem(tmp, sizeof(tmp));
   DO(ofb_decrypt(ct, tmp, 64, &ofb));
   if (memcmp(tmp, pt, 64) != 0) {
      fprintf(stderr, "OFB failed");
      return 1;
   }
#endif

#ifdef LTC_CTR_MODE   
   DO(ctr_test());
#endif

#ifdef LTC_XTS_MODE
   DO(xts_test());
#endif
         
   return 0;
}

/* $Source: /cvs/libtom/libtomcrypt/testprof/modes_test.c,v $ */
/* $Revision: 1.15 $ */
/* $Date: 2007/02/16 16:36:25 $ */
