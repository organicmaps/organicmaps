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

/** 
   @file pmac_memory.c
   PMAC implementation, process a block of memory, by Tom St Denis 
*/

#ifdef LTC_PMAC

/**
   PMAC a block of memory
   @param cipher   The index of the cipher desired
   @param key      The secret key
   @param keylen   The length of the secret key (octets)
   @param in       The data you wish to send through PMAC
   @param inlen    The length of data you wish to send through PMAC (octets)
   @param out      [out] Destination for the authentication tag
   @param outlen   [in/out] The max size and resulting size of the authentication tag
   @return CRYPT_OK if successful
*/
int pmac_memory(int cipher, 
                const unsigned char *key, unsigned long keylen,
                const unsigned char *in, unsigned long inlen,
                      unsigned char *out, unsigned long *outlen)
{
   int err;
   pmac_state *pmac;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(in    != NULL);
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
   if ((err = pmac_process(pmac, in, inlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   if ((err = pmac_done(pmac, out, outlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(pmac, sizeof(pmac_state));
#endif

   XFREE(pmac);
   return err;   
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
