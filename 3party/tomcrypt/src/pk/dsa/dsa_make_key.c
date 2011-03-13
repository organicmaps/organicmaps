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
   @file dsa_make_key.c
   DSA implementation, generate a DSA key, Tom St Denis
*/

#ifdef LTC_MDSA

/**
  Create a DSA key
  @param prng          An active PRNG state
  @param wprng         The index of the PRNG desired
  @param group_size    Size of the multiplicative group (octets)
  @param modulus_size  Size of the modulus (octets)
  @param key           [out] Where to store the created key
  @return CRYPT_OK if successful, upon error this function will free all allocated memory
*/
int dsa_make_key(prng_state *prng, int wprng, int group_size, int modulus_size, dsa_key *key)
{
   void           *tmp, *tmp2;
   int            err, res;
   unsigned char *buf;

   LTC_ARGCHK(key  != NULL);
   LTC_ARGCHK(ltc_mp.name != NULL);

   /* check prng */
   if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
      return err;
   }

   /* check size */
   if (group_size >= LTC_MDSA_MAX_GROUP || group_size <= 15 || 
       group_size >= modulus_size || (modulus_size - group_size) >= LTC_MDSA_DELTA) {
      return CRYPT_INVALID_ARG;
   }

   /* allocate ram */
   buf = XMALLOC(LTC_MDSA_DELTA);
   if (buf == NULL) {
      return CRYPT_MEM;
   }

   /* init mp_ints  */
   if ((err = mp_init_multi(&tmp, &tmp2, &key->g, &key->q, &key->p, &key->x, &key->y, NULL)) != CRYPT_OK) {
      XFREE(buf);
      return err;
   }

   /* make our prime q */
   if ((err = rand_prime(key->q, group_size, prng, wprng)) != CRYPT_OK)                { goto error; }

   /* double q  */
   if ((err = mp_add(key->q, key->q, tmp)) != CRYPT_OK)                                { goto error; }

   /* now make a random string and multply it against q */
   if (prng_descriptor[wprng].read(buf+1, modulus_size - group_size, prng) != (unsigned long)(modulus_size - group_size)) {
      err = CRYPT_ERROR_READPRNG;
      goto error;
   }

   /* force magnitude */
   buf[0] |= 0xC0;

   /* force even */
   buf[modulus_size - group_size - 1] &= ~1;

   if ((err = mp_read_unsigned_bin(tmp2, buf, modulus_size - group_size)) != CRYPT_OK) { goto error; }
   if ((err = mp_mul(key->q, tmp2, key->p)) != CRYPT_OK)                               { goto error; }
   if ((err = mp_add_d(key->p, 1, key->p)) != CRYPT_OK)                                { goto error; }

   /* now loop until p is prime */
   for (;;) {
       if ((err = mp_prime_is_prime(key->p, 8, &res)) != CRYPT_OK)                     { goto error; }
       if (res == LTC_MP_YES) break;

       /* add 2q to p and 2 to tmp2 */
       if ((err = mp_add(tmp, key->p, key->p)) != CRYPT_OK)                            { goto error; }
       if ((err = mp_add_d(tmp2, 2, tmp2)) != CRYPT_OK)                                { goto error; }
   }

   /* now p = (q * tmp2) + 1 is prime, find a value g for which g^tmp2 != 1 */
   mp_set(key->g, 1);

   do {
      if ((err = mp_add_d(key->g, 1, key->g)) != CRYPT_OK)                             { goto error; }
      if ((err = mp_exptmod(key->g, tmp2, key->p, tmp)) != CRYPT_OK)                   { goto error; }
   } while (mp_cmp_d(tmp, 1) == LTC_MP_EQ);

   /* at this point tmp generates a group of order q mod p */
   mp_exch(tmp, key->g);

   /* so now we have our DH structure, generator g, order q, modulus p 
      Now we need a random exponent [mod q] and it's power g^x mod p 
    */
   do {
      if (prng_descriptor[wprng].read(buf, group_size, prng) != (unsigned long)group_size) {
         err = CRYPT_ERROR_READPRNG;
         goto error;
      }
      if ((err = mp_read_unsigned_bin(key->x, buf, group_size)) != CRYPT_OK)           { goto error; }
   } while (mp_cmp_d(key->x, 1) != LTC_MP_GT);
   if ((err = mp_exptmod(key->g, key->x, key->p, key->y)) != CRYPT_OK)                 { goto error; }
  
   key->type = PK_PRIVATE;
   key->qord = group_size;

#ifdef LTC_CLEAN_STACK
   zeromem(buf, LTC_MDSA_DELTA);
#endif

   err = CRYPT_OK;
   goto done;
error: 
    mp_clear_multi(key->g, key->q, key->p, key->x, key->y, NULL);
done: 
    mp_clear_multi(tmp, tmp2, NULL);
    XFREE(buf);
    return err;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/dsa/dsa_make_key.c,v $ */
/* $Revision: 1.12 $ */
/* $Date: 2007/05/12 14:32:35 $ */
