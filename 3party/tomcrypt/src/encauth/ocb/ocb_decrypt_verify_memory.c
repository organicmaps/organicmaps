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
  @file ocb_decrypt_verify_memory.c
  OCB implementation, helper to decrypt block of memory, by Tom St Denis 
*/
#include "tomcrypt.h"

#ifdef LTC_OCB_MODE

/**
   Decrypt and compare the tag with OCB.
   @param cipher     The index of the cipher desired
   @param key        The secret key
   @param keylen     The length of the secret key (octets)
   @param nonce      The session nonce (length of the block size of the block cipher)
   @param ct         The ciphertext
   @param ctlen      The length of the ciphertext (octets)
   @param pt         [out] The plaintext
   @param tag        The tag to compare against
   @param taglen     The length of the tag (octets)
   @param stat       [out] The result of the tag comparison (1==valid, 0==invalid)
   @return CRYPT_OK if successful regardless of the tag comparison
*/
int ocb_decrypt_verify_memory(int cipher,
    const unsigned char *key,    unsigned long keylen,
    const unsigned char *nonce,  
    const unsigned char *ct,     unsigned long ctlen,
          unsigned char *pt,
    const unsigned char *tag,    unsigned long taglen,
          int           *stat)
{
   int err;
   ocb_state *ocb;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(nonce  != NULL);
   LTC_ARGCHK(pt     != NULL);
   LTC_ARGCHK(ct     != NULL);
   LTC_ARGCHK(tag    != NULL);
   LTC_ARGCHK(stat    != NULL);

   /* allocate memory */
   ocb = XMALLOC(sizeof(ocb_state));
   if (ocb == NULL) {
      return CRYPT_MEM;
   }

   if ((err = ocb_init(ocb, cipher, key, keylen, nonce)) != CRYPT_OK) {
      goto LBL_ERR; 
   }

   while (ctlen > (unsigned long)ocb->block_len) {
        if ((err = ocb_decrypt(ocb, ct, pt)) != CRYPT_OK) {
            goto LBL_ERR; 
        }
        ctlen   -= ocb->block_len;
        pt      += ocb->block_len;
        ct      += ocb->block_len;
   }

   err = ocb_done_decrypt(ocb, ct, ctlen, pt, tag, taglen, stat);
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(ocb, sizeof(ocb_state));
#endif
 
   XFREE(ocb);

   return err;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/ocb/ocb_decrypt_verify_memory.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */
