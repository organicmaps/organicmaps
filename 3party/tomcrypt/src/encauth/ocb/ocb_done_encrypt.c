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
   @file ocb_done_encrypt.c
   OCB implementation, terminate encryption, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_OCB_MODE

/** 
   Terminate an encryption OCB state
   @param ocb       The OCB state
   @param pt        Remaining plaintext (if any)
   @param ptlen     The length of the plaintext (octets)
   @param ct        [out] The ciphertext (if any)
   @param tag       [out] The tag for the OCB stream
   @param taglen    [in/out] The max size and resulting size of the tag
   @return CRYPT_OK if successful
*/
int ocb_done_encrypt(ocb_state *ocb, const unsigned char *pt, unsigned long ptlen,
                     unsigned char *ct, unsigned char *tag, unsigned long *taglen)
{
   LTC_ARGCHK(ocb    != NULL);
   LTC_ARGCHK(pt     != NULL);
   LTC_ARGCHK(ct     != NULL);
   LTC_ARGCHK(tag    != NULL);
   LTC_ARGCHK(taglen != NULL);
   return s_ocb_done(ocb, pt, ptlen, ct, tag, taglen, 0);
}

#endif


/* $Source: /cvs/libtom/libtomcrypt/src/encauth/ocb/ocb_done_encrypt.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */
