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
   @file f8_test_mode.c
   F8 implementation, test, Tom St Denis
*/


#ifdef LTC_F8_MODE

int f8_test_mode(void)
{
#ifndef LTC_TEST
   return CRYPT_NOP;
#else
   static const unsigned char key[16] = { 0x23, 0x48, 0x29, 0x00, 0x84, 0x67, 0xbe, 0x18, 
                                          0x6c, 0x3d, 0xe1, 0x4a, 0xae, 0x72, 0xd6, 0x2c };
   static const unsigned char salt[4] = { 0x32, 0xf2, 0x87, 0x0d };
   static const unsigned char IV[16]  = { 0x00, 0x6e, 0x5c, 0xba, 0x50, 0x68, 0x1d, 0xe5, 
                                          0x5c, 0x62, 0x15, 0x99, 0xd4, 0x62, 0x56, 0x4a };
   static const unsigned char pt[39]  = { 0x70, 0x73, 0x65, 0x75, 0x64, 0x6f, 0x72, 0x61, 
                                          0x6e, 0x64, 0x6f, 0x6d, 0x6e, 0x65, 0x73, 0x73,
                                          0x20, 0x69, 0x73, 0x20, 0x74, 0x68, 0x65, 0x20, 
                                          0x6e, 0x65, 0x78, 0x74, 0x20, 0x62, 0x65, 0x73,
                                          0x74, 0x20, 0x74, 0x68, 0x69, 0x6e, 0x67       };
   static const unsigned char ct[39]  = { 0x01, 0x9c, 0xe7, 0xa2, 0x6e, 0x78, 0x54, 0x01, 
                                          0x4a, 0x63, 0x66, 0xaa, 0x95, 0xd4, 0xee, 0xfd,
                                          0x1a, 0xd4, 0x17, 0x2a, 0x14, 0xf9, 0xfa, 0xf4, 
                                          0x55, 0xb7, 0xf1, 0xd4, 0xb6, 0x2b, 0xd0, 0x8f,
                                          0x56, 0x2c, 0x0e, 0xef, 0x7c, 0x48, 0x02       };
   unsigned char buf[39];
   symmetric_F8  f8;
   int           err, idx;
   
   idx = find_cipher("aes");
   if (idx == -1) {
      idx = find_cipher("rijndael");
      if (idx == -1) return CRYPT_NOP;
   }      
   
   /* initialize the context */
   if ((err = f8_start(idx, IV, key, sizeof(key), salt, sizeof(salt), 0, &f8)) != CRYPT_OK) {
      return err;
   }
   
   /* encrypt block */
   if ((err = f8_encrypt(pt, buf, sizeof(pt), &f8)) != CRYPT_OK) {
      f8_done(&f8);
      return err;
   }
   f8_done(&f8);

   /* compare */
   if (XMEMCMP(buf, ct, sizeof(ct))) {
      return CRYPT_FAIL_TESTVECTOR;
   }      
   
   return CRYPT_OK;
#endif   
}   

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
