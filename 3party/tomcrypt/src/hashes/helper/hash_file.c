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

#ifndef LTC_NO_FILE
/** 
  @file hash_file.c
  Hash a file, Tom St Denis
*/

/**
  @param hash   The index of the hash desired
  @param fname  The name of the file you wish to hash
  @param out    [out] The destination of the digest
  @param outlen [in/out] The max size and resulting size of the message digest
  @result CRYPT_OK if successful
*/
int hash_file(int hash, const char *fname, unsigned char *out, unsigned long *outlen)
{
    FILE *in;
    int err;
    LTC_ARGCHK(fname  != NULL);
    LTC_ARGCHK(out    != NULL);
    LTC_ARGCHK(outlen != NULL);

    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    in = fopen(fname, "rb");
    if (in == NULL) { 
       return CRYPT_FILE_NOTFOUND;
    }

    err = hash_filehandle(hash, in, out, outlen);
    if (fclose(in) != 0) {
       return CRYPT_ERROR;
    }

    return err;
}
#endif /* #ifndef LTC_NO_FILE */


/* $Source$ */
/* $Revision$ */
/* $Date$ */
