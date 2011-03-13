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
   @file ocb_done_decrypt.c
   OCB implementation, terminate decryption, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_OCB_MODE

/**
   Terminate a decrypting OCB state
   @param ocb    The OCB state
   @param ct     The ciphertext (if any)
   @param ctlen  The length of the ciphertext (octets)
   @param pt     [out] The plaintext
   @param tag    The authentication tag (to compare against)
   @param taglen The length of the authentication tag provided
   @param stat    [out] The result of the tag comparison
   @return CRYPT_OK if the process was successful regardless if the tag is valid
*/
int ocb_done_decrypt(ocb_state *ocb, 
                     const unsigned char *ct,  unsigned long ctlen,
                           unsigned char *pt, 
                     const unsigned char *tag, unsigned long taglen, int *stat)
{
   int err;
   unsigned char *tagbuf;
   unsigned long tagbuflen;

   LTC_ARGCHK(ocb  != NULL);
   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(tag  != NULL);
   LTC_ARGCHK(stat != NULL);

   /* default to failed */
   *stat = 0;

   /* allocate memory */
   tagbuf = XMALLOC(MAXBLOCKSIZE);
   if (tagbuf == NULL) {
      return CRYPT_MEM;
   }

   tagbuflen = MAXBLOCKSIZE;
   if ((err = s_ocb_done(ocb, ct, ctlen, pt, tagbuf, &tagbuflen, 1)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   if (taglen <= tagbuflen && XMEMCMP(tagbuf, tag, taglen) == 0) {
      *stat = 1;
   }

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(tagbuf, MAXBLOCKSIZE);
#endif

   XFREE(tagbuf);

   return err;
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/encauth/ocb/ocb_done_decrypt.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */
