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
  @file f9_memory_multi.c
  f9 support, process multiple blocks of memory, Tom St Denis
*/

#ifdef LTC_F9_MODE

/**
   f9 multiple blocks of memory 
   @param cipher    The index of the desired cipher
   @param key       The secret key
   @param keylen    The length of the secret key (octets)
   @param out       [out] The destination of the authentication tag
   @param outlen    [in/out]  The max size and resulting size of the authentication tag (octets)
   @param in        The data to send through f9
   @param inlen     The length of the data to send through f9 (octets)
   @param ...       tuples of (data,len) pairs to f9, terminated with a (NULL,x) (x=don't care)
   @return CRYPT_OK if successful
*/
int f9_memory_multi(int cipher, 
                const unsigned char *key, unsigned long keylen,
                      unsigned char *out, unsigned long *outlen,
                const unsigned char *in,  unsigned long inlen, ...)
{
   int                  err;
   f9_state          *f9;
   va_list              args;
   const unsigned char *curptr;
   unsigned long        curlen;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* allocate ram for f9 state */
   f9 = XMALLOC(sizeof(f9_state));
   if (f9 == NULL) {
      return CRYPT_MEM;
   }

   /* f9 process the message */
   if ((err = f9_init(f9, cipher, key, keylen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   va_start(args, inlen);
   curptr = in; 
   curlen = inlen;
   for (;;) {
      /* process buf */
      if ((err = f9_process(f9, curptr, curlen)) != CRYPT_OK) {
         goto LBL_ERR;
      }
      /* step to next */
      curptr = va_arg(args, const unsigned char*);
      if (curptr == NULL) {
         break;
      }
      curlen = va_arg(args, unsigned long);
   }
   if ((err = f9_done(f9, out, outlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(f9, sizeof(f9_state));
#endif
   XFREE(f9);
   va_end(args);
   return err;   
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/mac/f9/f9_memory_multi.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2006/12/28 01:27:23 $ */
