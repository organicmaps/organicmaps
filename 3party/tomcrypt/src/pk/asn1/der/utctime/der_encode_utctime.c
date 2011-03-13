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
  @file der_encode_utctime.c
  ASN.1 DER, encode a  UTCTIME, Tom St Denis
*/

#ifdef LTC_DER

static const char *baseten = "0123456789";

#define STORE_V(y) \
    out[x++] = der_ia5_char_encode(baseten[(y/10) % 10]); \
    out[x++] = der_ia5_char_encode(baseten[y % 10]);

/**
  Encodes a UTC time structure in DER format
  @param utctime      The UTC time structure to encode
  @param out          The destination of the DER encoding of the UTC time structure
  @param outlen       [in/out] The length of the DER encoding
  @return CRYPT_OK if successful
*/
int der_encode_utctime(ltc_utctime *utctime, 
                       unsigned char *out,   unsigned long *outlen)
{
    unsigned long x, tmplen;
    int           err;
 
    LTC_ARGCHK(utctime != NULL);
    LTC_ARGCHK(out     != NULL);
    LTC_ARGCHK(outlen  != NULL);

    if ((err = der_length_utctime(utctime, &tmplen)) != CRYPT_OK) {
       return err;
    }
    if (tmplen > *outlen) {
        *outlen = tmplen;
        return CRYPT_BUFFER_OVERFLOW;
    }
    
    /* store header */
    out[0] = 0x17;

    /* store values */
    x = 2;
    STORE_V(utctime->YY);
    STORE_V(utctime->MM);
    STORE_V(utctime->DD);
    STORE_V(utctime->hh);
    STORE_V(utctime->mm);
    STORE_V(utctime->ss);

    if (utctime->off_mm || utctime->off_hh) {
       out[x++] = der_ia5_char_encode(utctime->off_dir ? '-' : '+');
       STORE_V(utctime->off_hh);
       STORE_V(utctime->off_mm);
    } else {
       out[x++] = der_ia5_char_encode('Z');
    }

    /* store length */
    out[1] = (unsigned char)(x - 2);
   
    /* all good let's return */
    *outlen = x;
    return CRYPT_OK;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/asn1/der/utctime/der_encode_utctime.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2006/12/28 01:27:24 $ */
