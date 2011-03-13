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
   @file gcm_reset.c
   GCM implementation, reset a used state so it can accept IV data, by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_GCM_MODE

/**
  Reset a GCM state to as if you just called gcm_init().  This saves the initialization time.
  @param gcm   The GCM state to reset
  @return CRYPT_OK on success
*/
int gcm_reset(gcm_state *gcm)
{
   LTC_ARGCHK(gcm != NULL);

   zeromem(gcm->buf, sizeof(gcm->buf));
   zeromem(gcm->X,   sizeof(gcm->X));
   gcm->mode     = LTC_GCM_MODE_IV;
   gcm->ivmode   = 0;
   gcm->buflen   = 0;
   gcm->totlen   = 0;
   gcm->pttotlen = 0;
  
   return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/gcm/gcm_reset.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */
