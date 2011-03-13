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
   @file pmac_memory_multi.c
   PMAC implementation, process multiple blocks of memory, by Tom St Denis 
*/

#ifdef LTC_PMAC

/**
   PMAC multiple blocks of memory
   @param cipher   The index of the cipher desired
   @param key      The secret key
   @param keylen   The length of the secret key (octets)
   @param out      [out] Destination for the authentication tag
   @param outlen   [in/out] The max size and resulting size of the authentication tag
   @param in       The data you wish to send through PMAC
   @param inlen    The length of data you wish to send through PMAC (octets)
   @param ...      tuples of (data,len) pairs to PMAC, terminated with a (NULL,x) (x=don't care)
   @return CRYPT_OK if successful
*/
int pmac_memory_multi(int cipher, 
                const unsigned char *key, unsigned long  keylen,
                      unsigned char *out, unsigned long *outlen,
                const unsigned char *in,  unsigned long  inlen, ...)
{
   int                  err;
   pmac_state          *pmac;
   va_list              args;
   const unsigned char *curptr;
   unsigned long        curlen;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* allocate ram for pmac state */
   pmac = XMALLOC(sizeof(pmac_state));
   if (pmac == NULL) {
      return CRYPT_MEM;
   }
   
   if ((err = pmac_init(pmac, cipher, key, keylen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   va_start(args, inlen);
   curptr = in; 
   curlen = inlen;
   for (;;) {
      /* process buf */
      if ((err = pmac_process(pmac, curptr, curlen)) != CRYPT_OK) {
         goto LBL_ERR;
      }
      /* step to next */
      curptr = va_arg(args, const unsigned char*);
      if (curptr == NULL) {
         break;
      }
      curlen = va_arg(args, unsigned long);
   }
   if ((err = pmac_done(pmac, out, outlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(pmac, sizeof(pmac_state));
#endif
   XFREE(pmac);
   va_end(args);
   return err;   
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/mac/pmac/pmac_memory_multi.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2006/12/28 01:27:23 $ */
