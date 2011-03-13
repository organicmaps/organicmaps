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
  @file pkcs_1_oaep_decode.c
  OAEP Padding for LTC_PKCS #1, Tom St Denis 
*/

#ifdef LTC_PKCS_1

/**
   LTC_PKCS #1 v2.00 OAEP decode
   @param msg              The encoded data to decode
   @param msglen           The length of the encoded data (octets)
   @param lparam           The session or system data (can be NULL)
   @param lparamlen        The length of the lparam
   @param modulus_bitlen   The bit length of the RSA modulus
   @param hash_idx         The index of the hash desired
   @param out              [out] Destination of decoding
   @param outlen           [in/out] The max size and resulting size of the decoding
   @param res              [out] Result of decoding, 1==valid, 0==invalid
   @return CRYPT_OK if successful (even if invalid)
*/
int pkcs_1_oaep_decode(const unsigned char *msg,    unsigned long msglen,
                       const unsigned char *lparam, unsigned long lparamlen,
                             unsigned long modulus_bitlen, int hash_idx,
                             unsigned char *out,    unsigned long *outlen,
                             int           *res)
{
   unsigned char *DB, *seed, *mask;
   unsigned long hLen, x, y, modulus_len;
   int           err;

   LTC_ARGCHK(msg    != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(res    != NULL);

   /* default to invalid packet */
   *res = 0;
   
   /* test valid hash */
   if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) { 
      return err;
   }
   hLen        = hash_descriptor[hash_idx].hashsize;
   modulus_len = (modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0);

   /* test hash/message size */
   if ((2*hLen >= (modulus_len - 2)) || (msglen != modulus_len)) {
      return CRYPT_PK_INVALID_SIZE;
   }

   /* allocate ram for DB/mask/salt of size modulus_len */
   DB   = XMALLOC(modulus_len);
   mask = XMALLOC(modulus_len);
   seed = XMALLOC(hLen);
   if (DB == NULL || mask == NULL || seed == NULL) {
      if (DB != NULL) {
         XFREE(DB);
      }
      if (mask != NULL) {
         XFREE(mask);
      }
      if (seed != NULL) {
         XFREE(seed);
      }
      return CRYPT_MEM;
   }

   /* ok so it's now in the form
  
      0x00  || maskedseed || maskedDB 
  
       1    ||   hLen     ||  modulus_len - hLen - 1
   
    */

   /* must have leading 0x00 byte */
   if (msg[0] != 0x00) {
      err = CRYPT_OK;
      goto LBL_ERR;
   }

   /* now read the masked seed */
   x = 1;
   XMEMCPY(seed, msg + x, hLen);
   x += hLen;

   /* now read the masked DB */
   XMEMCPY(DB, msg + x, modulus_len - hLen - 1);
   x += modulus_len - hLen - 1;

   /* compute MGF1 of maskedDB (hLen) */ 
   if ((err = pkcs_1_mgf1(hash_idx, DB, modulus_len - hLen - 1, mask, hLen)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   /* XOR against seed */
   for (y = 0; y < hLen; y++) {
      seed[y] ^= mask[y];
   }

   /* compute MGF1 of seed (k - hlen - 1) */
   if ((err = pkcs_1_mgf1(hash_idx, seed, hLen, mask, modulus_len - hLen - 1)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   /* xor against DB */
   for (y = 0; y < (modulus_len - hLen - 1); y++) {
       DB[y] ^= mask[y]; 
   }

   /* now DB == lhash || PS || 0x01 || M, PS == k - mlen - 2hlen - 2 zeroes */

   /* compute lhash and store it in seed [reuse temps!] */
   x = modulus_len;
   if (lparam != NULL) {
      if ((err = hash_memory(hash_idx, lparam, lparamlen, seed, &x)) != CRYPT_OK) {
         goto LBL_ERR;
      }
   } else {
      /* can't pass hash_memory a NULL so use DB with zero length */
      if ((err = hash_memory(hash_idx, DB, 0, seed, &x)) != CRYPT_OK) {
         goto LBL_ERR;
      }
   }

   /* compare the lhash'es */
   if (XMEMCMP(seed, DB, hLen) != 0) {
      err = CRYPT_OK;
      goto LBL_ERR;
   }

   /* now zeroes before a 0x01 */
   for (x = hLen; x < (modulus_len - hLen - 1) && DB[x] == 0x00; x++) {
      /* step... */
   }

   /* error out if wasn't 0x01 */
   if (x == (modulus_len - hLen - 1) || DB[x] != 0x01) {
      err = CRYPT_INVALID_PACKET;
      goto LBL_ERR;
   }

   /* rest is the message (and skip 0x01) */
   if ((modulus_len - hLen - 1 - ++x) > *outlen) {
      *outlen = modulus_len - hLen - 1 - x;
      err = CRYPT_BUFFER_OVERFLOW;
      goto LBL_ERR;
   }

   /* copy message */
   *outlen = modulus_len - hLen - 1 - x;
   XMEMCPY(out, DB + x, modulus_len - hLen - 1 - x);
   x += modulus_len - hLen - 1;

   /* valid packet */
   *res = 1;

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(DB,   modulus_len);
   zeromem(seed, hLen);
   zeromem(mask, modulus_len);
#endif

   XFREE(seed);
   XFREE(mask);
   XFREE(DB);

   return err;
}

#endif /* LTC_PKCS_1 */

/* $Source: /cvs/libtom/libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_decode.c,v $ */
/* $Revision: 1.13 $ */
/* $Date: 2007/05/12 14:32:35 $ */
