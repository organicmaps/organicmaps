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
  @file omac_memory_multi.c
  LTC_OMAC1 support, process multiple blocks of memory, Tom St Denis
*/

#ifdef LTC_OMAC

/**
   LTC_OMAC multiple blocks of memory 
   @param cipher    The index of the desired cipher
   @param key       The secret key
   @param keylen    The length of the secret key (octets)
   @param out       [out] The destination of the authentication tag
   @param outlen    [in/out]  The max size and resulting size of the authentication tag (octets)
   @param in        The data to send through LTC_OMAC
   @param inlen     The length of the data to send through LTC_OMAC (octets)
   @param ...       tuples of (data,len) pairs to LTC_OMAC, terminated with a (NULL,x) (x=don't care)
   @return CRYPT_OK if successful
*/
int omac_memory_multi(int cipher, 
                const unsigned char *key, unsigned long keylen,
                      unsigned char *out, unsigned long *outlen,
                const unsigned char *in,  unsigned long inlen, ...)
{
   int                  err;
   omac_state          *omac;
   va_list              args;
   const unsigned char *curptr;
   unsigned long        curlen;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* allocate ram for omac state */
   omac = XMALLOC(sizeof(omac_state));
   if (omac == NULL) {
      return CRYPT_MEM;
   }

   /* omac process the message */
   if ((err = omac_init(omac, cipher, key, keylen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   va_start(args, inlen);
   curptr = in; 
   curlen = inlen;
   for (;;) {
      /* process buf */
      if ((err = omac_process(omac, curptr, curlen)) != CRYPT_OK) {
         goto LBL_ERR;
      }
      /* step to next */
      curptr = va_arg(args, const unsigned char*);
      if (curptr == NULL) {
         break;
      }
      curlen = va_arg(args, unsigned long);
   }
   if ((err = omac_done(omac, out, outlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(omac, sizeof(omac_state));
#endif
   XFREE(omac);
   va_end(args);
   return err;   
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/mac/omac/omac_memory_multi.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:37:41 $ */
