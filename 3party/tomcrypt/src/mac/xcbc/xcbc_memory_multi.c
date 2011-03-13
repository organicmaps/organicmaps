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
  @file xcbc_memory_multi.c
  XCBC support, process multiple blocks of memory, Tom St Denis
*/

#ifdef LTC_XCBC

/**
   XCBC multiple blocks of memory 
   @param cipher    The index of the desired cipher
   @param key       The secret key
   @param keylen    The length of the secret key (octets)
   @param out       [out] The destination of the authentication tag
   @param outlen    [in/out]  The max size and resulting size of the authentication tag (octets)
   @param in        The data to send through XCBC
   @param inlen     The length of the data to send through XCBC (octets)
   @param ...       tuples of (data,len) pairs to XCBC, terminated with a (NULL,x) (x=don't care)
   @return CRYPT_OK if successful
*/
int xcbc_memory_multi(int cipher, 
                const unsigned char *key, unsigned long keylen,
                      unsigned char *out, unsigned long *outlen,
                const unsigned char *in,  unsigned long inlen, ...)
{
   int                  err;
   xcbc_state          *xcbc;
   va_list              args;
   const unsigned char *curptr;
   unsigned long        curlen;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* allocate ram for xcbc state */
   xcbc = XMALLOC(sizeof(xcbc_state));
   if (xcbc == NULL) {
      return CRYPT_MEM;
   }

   /* xcbc process the message */
   if ((err = xcbc_init(xcbc, cipher, key, keylen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   va_start(args, inlen);
   curptr = in; 
   curlen = inlen;
   for (;;) {
      /* process buf */
      if ((err = xcbc_process(xcbc, curptr, curlen)) != CRYPT_OK) {
         goto LBL_ERR;
      }
      /* step to next */
      curptr = va_arg(args, const unsigned char*);
      if (curptr == NULL) {
         break;
      }
      curlen = va_arg(args, unsigned long);
   }
   if ((err = xcbc_done(xcbc, out, outlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(xcbc, sizeof(xcbc_state));
#endif
   XFREE(xcbc);
   va_end(args);
   return err;   
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/mac/xcbc/xcbc_memory_multi.c,v $ */
/* $Revision: 1.2 $ */
/* $Date: 2006/12/28 01:27:23 $ */
