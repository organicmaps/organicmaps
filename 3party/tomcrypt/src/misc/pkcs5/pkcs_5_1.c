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
#include <tomcrypt.h>

/** 
   @file pkcs_5_1.c
   LTC_PKCS #5, Algorithm #1, Tom St Denis
*/
#ifdef LTC_PKCS_5
/**
   Execute LTC_PKCS #5 v1
   @param password         The password (or key)
   @param password_len     The length of the password (octet)
   @param salt             The salt (or nonce) which is 8 octets long
   @param iteration_count  The LTC_PKCS #5 v1 iteration count
   @param hash_idx         The index of the hash desired
   @param out              [out] The destination for this algorithm
   @param outlen           [in/out] The max size and resulting size of the algorithm output
   @return CRYPT_OK if successful
*/
int pkcs_5_alg1(const unsigned char *password, unsigned long password_len, 
                const unsigned char *salt, 
                int iteration_count,  int hash_idx,
                unsigned char *out,   unsigned long *outlen)
{
   int err;
   unsigned long x;
   hash_state    *md;
   unsigned char *buf;

   LTC_ARGCHK(password != NULL);
   LTC_ARGCHK(salt     != NULL);
   LTC_ARGCHK(out      != NULL);
   LTC_ARGCHK(outlen   != NULL);

   /* test hash IDX */
   if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
      return err;
   }

   /* allocate memory */
   md  = XMALLOC(sizeof(hash_state));
   buf = XMALLOC(MAXBLOCKSIZE);
   if (md == NULL || buf == NULL) {
      if (md != NULL) {
         XFREE(md);
      }
      if (buf != NULL) { 
         XFREE(buf);
      }
      return CRYPT_MEM;
   }        

   /* hash initial password + salt */
   if ((err = hash_descriptor[hash_idx].init(md)) != CRYPT_OK) {
       goto LBL_ERR;
   }
   if ((err = hash_descriptor[hash_idx].process(md, password, password_len)) != CRYPT_OK) {
       goto LBL_ERR;
   }
   if ((err = hash_descriptor[hash_idx].process(md, salt, 8)) != CRYPT_OK) {
       goto LBL_ERR;
   }
   if ((err = hash_descriptor[hash_idx].done(md, buf)) != CRYPT_OK) {
       goto LBL_ERR;
   }

   while (--iteration_count) {
      /* code goes here. */
      x = MAXBLOCKSIZE;
      if ((err = hash_memory(hash_idx, buf, hash_descriptor[hash_idx].hashsize, buf, &x)) != CRYPT_OK) {
         goto LBL_ERR;
      }
   }

   /* copy upto outlen bytes */
   for (x = 0; x < hash_descriptor[hash_idx].hashsize && x < *outlen; x++) {
       out[x] = buf[x];
   }
   *outlen = x;
   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK 
   zeromem(buf, MAXBLOCKSIZE);
   zeromem(md, sizeof(hash_state));
#endif

   XFREE(buf);
   XFREE(md);

   return err;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/misc/pkcs5/pkcs_5_1.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */
