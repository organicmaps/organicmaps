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

/**
   @file eax_done.c
   EAX implementation, terminate session, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_EAX_MODE

/**
   Terminate an EAX session and get the tag.
   @param eax       The EAX state
   @param tag       [out] The destination of the authentication tag
   @param taglen    [in/out] The max length and resulting length of the authentication tag
   @return CRYPT_OK if successful
*/
int eax_done(eax_state *eax, unsigned char *tag, unsigned long *taglen)
{
   int           err;
   unsigned char *headermac, *ctmac;
   unsigned long x, len;

   LTC_ARGCHK(eax    != NULL);
   LTC_ARGCHK(tag    != NULL);
   LTC_ARGCHK(taglen != NULL);

   /* allocate ram */
   headermac = XMALLOC(MAXBLOCKSIZE);
   ctmac     = XMALLOC(MAXBLOCKSIZE);

   if (headermac == NULL || ctmac == NULL) {
      if (headermac != NULL) {
         XFREE(headermac);
      }
      if (ctmac != NULL) {
         XFREE(ctmac);
      }
      return CRYPT_MEM;
   }

   /* finish ctomac */
   len = MAXBLOCKSIZE;
   if ((err = omac_done(&eax->ctomac, ctmac, &len)) != CRYPT_OK) {
      goto LBL_ERR; 
   }

   /* finish headeromac */

   /* note we specifically don't reset len so the two lens are minimal */

   if ((err = omac_done(&eax->headeromac, headermac, &len)) != CRYPT_OK) {
      goto LBL_ERR; 
   }

   /* terminate the CTR chain */
   if ((err = ctr_done(&eax->ctr)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   /* compute N xor H xor C */
   for (x = 0; x < len && x < *taglen; x++) {
       tag[x] = eax->N[x] ^ headermac[x] ^ ctmac[x];
   }
   *taglen = x;

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(ctmac,     MAXBLOCKSIZE);
   zeromem(headermac, MAXBLOCKSIZE);
   zeromem(eax,       sizeof(*eax));
#endif

   XFREE(ctmac);
   XFREE(headermac);

   return err;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/eax/eax_done.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */
