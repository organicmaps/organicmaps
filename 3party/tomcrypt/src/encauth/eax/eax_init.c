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
   @file eax_init.c
   EAX implementation, initialized EAX state, by Tom St Denis 
*/
#include "tomcrypt.h"

#ifdef LTC_EAX_MODE

/** 
   Initialized an EAX state
   @param eax       [out] The EAX state to initialize
   @param cipher    The index of the desired cipher
   @param key       The secret key
   @param keylen    The length of the secret key (octets)
   @param nonce     The use-once nonce for the session
   @param noncelen  The length of the nonce (octets)
   @param header    The header for the EAX state
   @param headerlen The header length (octets)
   @return CRYPT_OK if successful
*/
int eax_init(eax_state *eax, int cipher, 
             const unsigned char *key,    unsigned long keylen,
             const unsigned char *nonce,  unsigned long noncelen,
             const unsigned char *header, unsigned long headerlen)
{
   unsigned char *buf;
   int           err, blklen;
   omac_state    *omac;
   unsigned long len;


   LTC_ARGCHK(eax   != NULL);
   LTC_ARGCHK(key   != NULL);
   LTC_ARGCHK(nonce != NULL);
   if (headerlen > 0) {
      LTC_ARGCHK(header != NULL);
   }

   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }
   blklen = cipher_descriptor[cipher].block_length;

   /* allocate ram */
   buf  = XMALLOC(MAXBLOCKSIZE);
   omac = XMALLOC(sizeof(*omac));

   if (buf == NULL || omac == NULL) {
      if (buf != NULL) {
         XFREE(buf);
      }
      if (omac != NULL) {
         XFREE(omac);
      }
      return CRYPT_MEM;
   }

   /* N = LTC_OMAC_0K(nonce) */
   zeromem(buf, MAXBLOCKSIZE);
   if ((err = omac_init(omac, cipher, key, keylen)) != CRYPT_OK) {
      goto LBL_ERR; 
   }

   /* omac the [0]_n */
   if ((err = omac_process(omac, buf, blklen)) != CRYPT_OK) {
      goto LBL_ERR; 
   }
   /* omac the nonce */
   if ((err = omac_process(omac, nonce, noncelen)) != CRYPT_OK) {
      goto LBL_ERR; 
   }
   /* store result */
   len = sizeof(eax->N);
   if ((err = omac_done(omac, eax->N, &len)) != CRYPT_OK) {
      goto LBL_ERR; 
   }

   /* H = LTC_OMAC_1K(header) */
   zeromem(buf, MAXBLOCKSIZE);
   buf[blklen - 1] = 1;

   if ((err = omac_init(&eax->headeromac, cipher, key, keylen)) != CRYPT_OK) {
      goto LBL_ERR; 
   }

   /* omac the [1]_n */
   if ((err = omac_process(&eax->headeromac, buf, blklen)) != CRYPT_OK) {
      goto LBL_ERR; 
   }
   /* omac the header */
   if (headerlen != 0) {
      if ((err = omac_process(&eax->headeromac, header, headerlen)) != CRYPT_OK) {
          goto LBL_ERR; 
      }
   }

   /* note we don't finish the headeromac, this allows us to add more header later */

   /* setup the CTR mode */
   if ((err = ctr_start(cipher, eax->N, key, keylen, 0, CTR_COUNTER_BIG_ENDIAN, &eax->ctr)) != CRYPT_OK) {
      goto LBL_ERR; 
   }

   /* setup the LTC_OMAC for the ciphertext */
   if ((err = omac_init(&eax->ctomac, cipher, key, keylen)) != CRYPT_OK) { 
      goto LBL_ERR; 
   }

   /* omac [2]_n */
   zeromem(buf, MAXBLOCKSIZE);
   buf[blklen-1] = 2;
   if ((err = omac_process(&eax->ctomac, buf, blklen)) != CRYPT_OK) {
      goto LBL_ERR; 
   }

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(buf,  MAXBLOCKSIZE);
   zeromem(omac, sizeof(*omac));
#endif

   XFREE(omac);
   XFREE(buf);

   return err;
}

#endif 

/* $Source$ */
/* $Revision$ */
/* $Date$ */
