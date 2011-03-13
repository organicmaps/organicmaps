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
    @file eax_addheader.c
    EAX implementation, add meta-data, by Tom St Denis 
*/
#include "tomcrypt.h"

#ifdef LTC_EAX_MODE

/** 
    add header (metadata) to the stream 
    @param eax    The current EAX state
    @param header The header (meta-data) data you wish to add to the state
    @param length The length of the header data
    @return CRYPT_OK if successful
*/
int eax_addheader(eax_state *eax, const unsigned char *header, 
                  unsigned long length)
{
   LTC_ARGCHK(eax    != NULL);
   LTC_ARGCHK(header != NULL);
   return omac_process(&eax->headeromac, header, length);
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/eax/eax_addheader.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */
