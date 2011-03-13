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
#include <stdarg.h>

/**
  @file crypt_fsa.c
  LibTomCrypt FULL SPEED AHEAD!, Tom St Denis
*/  

/* format is ltc_mp, cipher_desc, [cipher_desc], NULL, hash_desc, [hash_desc], NULL, prng_desc, [prng_desc], NULL */
int crypt_fsa(void *mp, ...)
{
   int      err;
   va_list  args;
   void     *p;

   va_start(args, mp);
   if (mp != NULL) {
      XMEMCPY(&ltc_mp, mp, sizeof(ltc_mp));
   }
   
   while ((p = va_arg(args, void*)) != NULL) {
      if ((err = register_cipher(p)) != CRYPT_OK) {
         va_end(args);
         return err;
      }
   }

   while ((p = va_arg(args, void*)) != NULL) {
      if ((err = register_hash(p)) != CRYPT_OK) {
         va_end(args);
         return err;
      }
   }

   while ((p = va_arg(args, void*)) != NULL) {
      if ((err = register_prng(p)) != CRYPT_OK) {
         va_end(args);
         return err;
      }
   }

   va_end(args);
   return CRYPT_OK;   
}


/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_fsa.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2006/12/28 01:27:24 $ */
