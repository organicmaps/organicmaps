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

/* Implements ECC over Z/pZ for curve y^2 = x^3 - 3x + b
 *
 * All curves taken from NIST recommendation paper of July 1999
 * Available at http://csrc.nist.gov/cryptval/dss.htm
 */
#include "tomcrypt.h"

/**
  @file ecc_shared_secret.c
  ECC Crypto, Tom St Denis
*/  

#ifdef LTC_MECC

/**
  Create an ECC shared secret between two keys
  @param private_key      The private ECC key
  @param public_key       The public key
  @param out              [out] Destination of the shared secret (Conforms to EC-DH from ANSI X9.63)
  @param outlen           [in/out] The max size and resulting size of the shared secret
  @return CRYPT_OK if successful
*/
int ecc_shared_secret(ecc_key *private_key, ecc_key *public_key,
                      unsigned char *out, unsigned long *outlen)
{
   unsigned long  x;
   ecc_point     *result;
   void          *prime;
   int            err;

   LTC_ARGCHK(private_key != NULL);
   LTC_ARGCHK(public_key  != NULL);
   LTC_ARGCHK(out         != NULL);
   LTC_ARGCHK(outlen      != NULL);

   /* type valid? */
   if (private_key->type != PK_PRIVATE) {
      return CRYPT_PK_NOT_PRIVATE;
   }

   if (ltc_ecc_is_valid_idx(private_key->idx) == 0 || ltc_ecc_is_valid_idx(public_key->idx) == 0) {
      return CRYPT_INVALID_ARG;
   }

   if (XSTRCMP(private_key->dp->name, public_key->dp->name) != 0) {
      return CRYPT_PK_TYPE_MISMATCH;
   }

   /* make new point */
   result = ltc_ecc_new_point();
   if (result == NULL) {
      return CRYPT_MEM;
   }

   if ((err = mp_init(&prime)) != CRYPT_OK) {
      ltc_ecc_del_point(result);
      return err;
   }

   if ((err = mp_read_radix(prime, (char *)private_key->dp->prime, 16)) != CRYPT_OK)                               { goto done; }
   if ((err = ltc_mp.ecc_ptmul(private_key->k, &public_key->pubkey, result, prime, 1)) != CRYPT_OK)                { goto done; }

   x = (unsigned long)mp_unsigned_bin_size(prime);
   if (*outlen < x) {
      *outlen = x;
      err = CRYPT_BUFFER_OVERFLOW;
      goto done;
   }
   zeromem(out, x);
   if ((err = mp_to_unsigned_bin(result->x, out + (x - mp_unsigned_bin_size(result->x))))   != CRYPT_OK)           { goto done; }

   err     = CRYPT_OK;
   *outlen = x;
done:
   mp_clear(prime);
   ltc_ecc_del_point(result);
   return err;
}

#endif
/* $Source: /cvs/libtom/libtomcrypt/src/pk/ecc/ecc_shared_secret.c,v $ */
/* $Revision: 1.10 $ */
/* $Date: 2007/05/12 14:32:35 $ */

