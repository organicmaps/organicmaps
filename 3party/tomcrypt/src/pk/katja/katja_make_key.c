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
  @file katja_make_key.c
  Katja key generation, Tom St Denis
*/  

#ifdef LTC_MKAT

/** 
   Create a Katja key
   @param prng     An active PRNG state
   @param wprng    The index of the PRNG desired
   @param size     The size of the modulus (key size) desired (octets)
   @param key      [out] Destination of a newly created private key pair
   @return CRYPT_OK if successful, upon error all allocated ram is freed
*/
int katja_make_key(prng_state *prng, int wprng, int size, katja_key *key)
{
   void *p, *q, *tmp1, *tmp2;
   int    err;
  
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(ltc_mp.name != NULL);

   if ((size < (MIN_KAT_SIZE/8)) || (size > (MAX_KAT_SIZE/8))) {
      return CRYPT_INVALID_KEYSIZE;
   }

   if ((err = prng_is_valid(wprng)) != CRYPT_OK) {
      return err;
   }

   if ((err = mp_init_multi(&p, &q, &tmp1, &tmp2, NULL)) != CRYPT_OK) {
      return err;
   }

   /* divide size by three  */
   size   = (((size << 3) / 3) + 7) >> 3;

   /* make prime "q" (we negate size to make q == 3 mod 4) */
   if ((err = rand_prime(q, -size, prng, wprng)) != CRYPT_OK)      { goto done; }
   if ((err = mp_sub_d(q, 1, tmp1)) != CRYPT_OK)                   { goto done; }

   /* make prime "p" */
   do {
      if ((err = rand_prime(p, size+1, prng, wprng)) != CRYPT_OK)  { goto done; }
      if ((err = mp_gcd(p, tmp1, tmp2)) != CRYPT_OK)               { goto done; }
   } while (mp_cmp_d(tmp2, 1) != LTC_MP_EQ);

   /* make key */
   if ((err = mp_init_multi(&key->d, &key->N, &key->dQ, &key->dP,
                     &key->qP, &key->p, &key->q, &key->pq, NULL)) != CRYPT_OK) {
      goto error;
   }

   /* n=p^2q and 1/n mod pq */
   if ((err = mp_copy( p,  key->p)) != CRYPT_OK)                       { goto error2; }
   if ((err = mp_copy( q,  key->q)) != CRYPT_OK)                       { goto error2; }
   if ((err = mp_mul(key->p, key->q, key->pq)) != CRYPT_OK)            { goto error2; } /* tmp1 = pq  */
   if ((err = mp_mul(key->pq, key->p, key->N)) != CRYPT_OK)            { goto error2; } /* N = p^2q   */  
   if ((err = mp_sub_d( p, 1,  tmp1)) != CRYPT_OK)                     { goto error2; } /* tmp1 = q-1 */
   if ((err = mp_sub_d( q, 1,  tmp2)) != CRYPT_OK)                     { goto error2; } /* tmp2 = p-1 */
   if ((err = mp_lcm(tmp1, tmp2, key->d)) != CRYPT_OK)                 { goto error2; } /* tmp1 = lcd(p-1,q-1) */
   if ((err = mp_invmod( key->N,  key->d,  key->d)) != CRYPT_OK)       { goto error2; } /* key->d = 1/N mod pq */

   /* optimize for CRT now */
   /* find d mod q-1 and d mod p-1 */
   if ((err = mp_mod( key->d,  tmp1,  key->dP)) != CRYPT_OK)           { goto error2; } /* dP = d mod p-1 */
   if ((err = mp_mod( key->d,  tmp2,  key->dQ)) != CRYPT_OK)           { goto error2; } /* dQ = d mod q-1 */
   if ((err = mp_invmod( q,  p,  key->qP)) != CRYPT_OK)                { goto error2; } /* qP = 1/q mod p */

   /* set key type (in this case it's CRT optimized) */
   key->type = PK_PRIVATE;

   /* return ok and free temps */
   err       = CRYPT_OK;
   goto done;
error2:
   mp_clear_multi( key->d,  key->N,  key->dQ,  key->dP,  key->qP,  key->p,  key->q, key->pq, NULL);
error:
done:
   mp_clear_multi( tmp2,  tmp1,  p,  q, NULL);
   return err;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
