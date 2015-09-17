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
 Source donated by Elliptic Semiconductor Inc (www.ellipticsemi.com) to the LibTom Projects
 */

#ifdef LTC_XTS_MODE

static int tweak_crypt(const unsigned char *P, unsigned char *C, unsigned char *T, symmetric_xts *xts)
{
   unsigned long x;
   int err;

   /* tweak encrypt block i */
#ifdef LTC_FAST
   for (x = 0; x < 16; x += sizeof(LTC_FAST_TYPE)) {
      *((LTC_FAST_TYPE *)&C[x]) = *((LTC_FAST_TYPE *)&P[x]) ^ *((LTC_FAST_TYPE *)&T[x]);
   }
#else
   for (x = 0; x < 16; x++) {
      C[x] = P[x] ^ T[x];
   }
#endif

   if ((err = cipher_descriptor[xts->cipher].ecb_encrypt(C, C, &xts->key1)) != CRYPT_OK) {
      return err;
   }

#ifdef LTC_FAST
   for (x = 0; x < 16; x += sizeof(LTC_FAST_TYPE)) {
      *((LTC_FAST_TYPE *)&C[x]) ^= *((LTC_FAST_TYPE *)&T[x]);
   }
#else
   for (x = 0; x < 16; x++) {
      C[x] = C[x] ^ T[x];
   }
#endif

   /* LFSR the tweak */
   xts_mult_x(T);

   return CRYPT_OK;
}

/** XTS Encryption
 @param pt     [in]  Plaintext
 @param ptlen  Length of plaintext (and ciphertext)
 @param ct     [out] Ciphertext
 @param tweak  [in] The 128--bit encryption tweak (e.g. sector number)
 @param xts    The XTS structure
 Returns CRYPT_OK upon success
 */
int xts_encrypt(const unsigned char *pt, unsigned long ptlen, unsigned char *ct, unsigned char *tweak,
                symmetric_xts *xts)
{
   unsigned char PP[16], CC[16], T[16];
   unsigned long i, m, mo, lim;
   int err;

   /* check inputs */
   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(tweak != NULL);
   LTC_ARGCHK(xts != NULL);

   /* check if valid */
   if ((err = cipher_is_valid(xts->cipher)) != CRYPT_OK) {
      return err;
   }

   /* get number of blocks */
   m = ptlen >> 4;
   mo = ptlen & 15;

   /* must have at least one full block */
   if (m == 0) {
      return CRYPT_INVALID_ARG;
   }

   if (mo == 0) {
      lim = m;
   } else {
      lim = m - 1;
   }

   if (cipher_descriptor[xts->cipher].accel_xts_encrypt && lim > 0) {

      /* use accelerated encryption for whole blocks */
      if ((err = cipher_descriptor[xts->cipher].accel_xts_encrypt(pt, ct, lim, tweak, &xts->key1, &xts->key2) !=
                 CRYPT_OK)) {
         return err;
      }
      ct += lim * 16;
      pt += lim * 16;

      /* tweak is encrypted on output */
      XMEMCPY(T, tweak, sizeof(T));
   } else {

      /* encrypt the tweak */
      if ((err = cipher_descriptor[xts->cipher].ecb_encrypt(tweak, T, &xts->key2)) != CRYPT_OK) {
         return err;
      }

      for (i = 0; i < lim; i++) {
         err = tweak_crypt(pt, ct, T, xts);
         ct += 16;
         pt += 16;
      }
   }

   /* if ptlen not divide 16 then */
   if (mo > 0) {
      /* CC = tweak encrypt block m-1 */
      if ((err = tweak_crypt(pt, CC, T, xts)) != CRYPT_OK) {
         return err;
      }

      /* Cm = first ptlen % 16 bytes of CC */
      for (i = 0; i < mo; i++) {
         PP[i] = pt[16 + i];
         ct[16 + i] = CC[i];
      }

      for (; i < 16; i++) {
         PP[i] = CC[i];
      }

      /* Cm-1 = Tweak encrypt PP */
      if ((err = tweak_crypt(PP, ct, T, xts)) != CRYPT_OK) {
         return err;
      }
   }

   /* Decrypt the tweak back */
   if ((err = cipher_descriptor[xts->cipher].ecb_decrypt(T, tweak, &xts->key2)) != CRYPT_OK) {
      return err;
   }

   return err;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
