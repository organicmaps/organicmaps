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
  @file hmac_file.c
  LTC_HMAC support, process a file, Tom St Denis/Dobes Vandermeer
*/

#ifdef LTC_HMAC

/**
  LTC_HMAC a file
  @param hash     The index of the hash you wish to use
  @param fname    The name of the file you wish to LTC_HMAC
  @param key      The secret key
  @param keylen   The length of the secret key
  @param out      [out] The LTC_HMAC authentication tag
  @param outlen   [in/out]  The max size and resulting size of the authentication tag
  @return CRYPT_OK if successful, CRYPT_NOP if file support has been disabled
*/
int hmac_file(int hash, const char *fname, 
              const unsigned char *key, unsigned long keylen, 
                    unsigned char *out, unsigned long *outlen)
{
#ifdef LTC_NO_FILE
    return CRYPT_NOP;
#else
   hmac_state hmac;
   FILE *in;
   unsigned char buf[512];
   size_t x;
   int err;

   LTC_ARGCHK(fname  != NULL);
   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
   
   if((err = hash_is_valid(hash)) != CRYPT_OK) {
       return err;
   }

   if ((err = hmac_init(&hmac, hash, key, keylen)) != CRYPT_OK) {
       return err;
   }

   in = fopen(fname, "rb");
   if (in == NULL) {
      return CRYPT_FILE_NOTFOUND;
   }

   /* process the file contents */
   do {
      x = fread(buf, 1, sizeof(buf), in);
      if ((err = hmac_process(&hmac, buf, (unsigned long)x)) != CRYPT_OK) {
         /* we don't trap this error since we're already returning an error! */
         fclose(in);
         return err;
      }
   } while (x == sizeof(buf));

   if (fclose(in) != 0) {
      return CRYPT_ERROR;
   }

   /* get final hmac */
   if ((err = hmac_done(&hmac, out, outlen)) != CRYPT_OK) {
      return err;
   }

#ifdef LTC_CLEAN_STACK
   /* clear memory */
   zeromem(buf, sizeof(buf));
#endif   
   return CRYPT_OK;
#endif
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/mac/hmac/hmac_file.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:37:41 $ */
