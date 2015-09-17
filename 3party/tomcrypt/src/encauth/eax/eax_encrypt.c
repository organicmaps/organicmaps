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
   @file eax_encrypt.c
   EAX implementation, encrypt block by Tom St Denis 
*/
#include "tomcrypt.h"

#ifdef LTC_EAX_MODE

/**
   Encrypt with EAX a block of data.
   @param eax        The EAX state
   @param pt         The plaintext to encrypt
   @param ct         [out] The ciphertext as encrypted
   @param length     The length of the plaintext (octets)
   @return CRYPT_OK if successful
*/
int eax_encrypt(eax_state *eax, const unsigned char *pt, unsigned char *ct, 
                unsigned long length)
{
   int err;
   
   LTC_ARGCHK(eax != NULL);
   LTC_ARGCHK(pt  != NULL);
   LTC_ARGCHK(ct  != NULL);

   /* encrypt */
   if ((err = ctr_encrypt(pt, ct, length, &eax->ctr)) != CRYPT_OK) {
      return err;
   }

   /* omac ciphertext */
   return omac_process(&eax->ctomac, ct, length);
}

#endif


/* $Source$ */
/* $Revision$ */
/* $Date$ */
