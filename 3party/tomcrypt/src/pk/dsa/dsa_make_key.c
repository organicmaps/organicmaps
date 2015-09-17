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
  Create DSA parameters
  @param prng          An active PRNG state
  @param wprng         The index of the PRNG desired
  @param group_size    Size of the multiplicative group (octets)
  @param modulus_size  Size of the modulus (octets)
  @param p             [out] bignum where generated 'p' is stored (must be initialized by caller)
  @param q             [out] bignum where generated 'q' is stored (must be initialized by caller)
  @param g             [out] bignum where generated 'g' is stored (must be initialized by caller)
  @return CRYPT_OK if successful, upon error this function will free all allocated memory
*/
int dsa_make_params(prng_state *prng, int wprng, int group_size, int modulus_size, void *p, void *q, void *g)
{
  unsigned long L, N, n, outbytes, seedbytes, counter, j, i;
  int err, res, mr_tests_q, mr_tests_p, found_p, found_q, hash;
  unsigned char *wbuf, *sbuf, digest[MAXBLOCKSIZE];
  void *t2L1, *t2N1, *t2q, *t2seedlen, *U, *W, *X, *c, *h, *e, *seedinc;

  /* check size */
  if (group_size >= LTC_MDSA_MAX_GROUP || group_size < 1 || group_size >= modulus_size) {
    return CRYPT_INVALID_ARG;
  }

 /* FIPS-186-4 A.1.1.2 Generation of the Probable Primes p and q Using an Approved Hash Function
  *
  * L = The desired length of the prime p (in bits e.g. L = 1024)
  * N = The desired length of the prime q (in bits e.g. N = 160)
  * seedlen = The desired bit length of the domain parameter seed; seedlen shallbe equal to or greater than N
  * outlen  = The bit length of Hash function
  *
  * 1.  Check that the (L, N)
  * 2.  If (seedlen <N), then return INVALID.
  * 3.  n = ceil(L / outlen) - 1
  * 4.  b = L- 1 - (n * outlen)
  * 5.  domain_parameter_seed = an arbitrary sequence of seedlen bits
  * 6.  U = Hash (domain_parameter_seed) mod 2^(N-1)
  * 7.  q = 2^(N-1) + U + 1 - (U mod 2)
  * 8.  Test whether or not q is prime as specified in Appendix C.3
  * 9.  If qis not a prime, then go to step 5.
  * 10. offset = 1
  * 11. For counter = 0 to (4L- 1) do {
  *       For j=0 to n do {
  *         Vj = Hash ((domain_parameter_seed+ offset + j) mod 2^seedlen
  *       }
  *       W = V0 + (V1 *2^outlen) + ... + (Vn-1 * 2^((n-1) * outlen)) + ((Vn mod 2^b) * 2^(n * outlen))
  *       X = W + 2^(L-1)           Comment: 0 <= W < 2^(L-1); hence 2^(L-1) <= X < 2^L
  *       c = X mod 2*q
  *       p = X - (c - 1)           Comment: p ~ 1 (mod 2*q)
  *       If (p >= 2^(L-1)) {
  *         Test whether or not p is prime as specified in Appendix C.3.
  *         If p is determined to be prime, then return VALID and the values of p, qand (optionally) the values of domain_parameter_seed and counter
  *       }
  *       offset = offset + n + 1   Comment: Increment offset
  *     }
  */

  seedbytes = group_size;
  L = modulus_size * 8;
  N = group_size * 8;

  /* M-R tests (when followed by one Lucas test) according FIPS-186-4 - Appendix C.3 - table C.1 */
  mr_tests_p = (L <= 2048) ? 3 : 2;
  if      (N <= 160)  { mr_tests_q = 19; }
  else if (N <= 224)  { mr_tests_q = 24; }
  else                { mr_tests_q = 27; }

  if (N <= 256) {
    hash = register_hash(&sha256_desc);
  }
  else if (N <= 384) {
    hash = register_hash(&sha384_desc);
  }
  else if (N <= 512) {
    hash = register_hash(&sha512_desc);
  }
  else {
    return CRYPT_INVALID_ARG; /* group_size too big */
  }

  if ((err = hash_is_valid(hash)) != CRYPT_OK)                                   { return err; }
  outbytes = hash_descriptor[hash].hashsize;

  n = ((L + outbytes*8 - 1) / (outbytes*8)) - 1;

  if ((wbuf = XMALLOC((n+1)*outbytes)) == NULL)                                  { err = CRYPT_MEM; goto cleanup3; }
  if ((sbuf = XMALLOC(seedbytes)) == NULL)                                       { err = CRYPT_MEM; goto cleanup2; }

  err = mp_init_multi(&t2L1, &t2N1, &t2q, &t2seedlen, &U, &W, &X, &c, &h, &e, &seedinc, NULL);
  if (err != CRYPT_OK)                                                           { goto cleanup1; }

  if ((err = mp_2expt(t2L1, L-1)) != CRYPT_OK)                                   { goto cleanup; }
  /* t2L1 = 2^(L-1) */
  if ((err = mp_2expt(t2N1, N-1)) != CRYPT_OK)                                   { goto cleanup; }
  /* t2N1 = 2^(N-1) */
  if ((err = mp_2expt(t2seedlen, seedbytes*8)) != CRYPT_OK)                      { goto cleanup; }
  /* t2seedlen = 2^seedlen */

  for(found_p=0; !found_p;) {
    /* q */
    for(found_q=0; !found_q;) {
      if (prng_descriptor[wprng].read(sbuf, seedbytes, prng) != seedbytes)       { err = CRYPT_ERROR_READPRNG; goto cleanup; }
      i = outbytes;
      if ((err = hash_memory(hash, sbuf, seedbytes, digest, &i)) != CRYPT_OK)    { goto cleanup; }
      if ((err = mp_read_unsigned_bin(U, digest, outbytes)) != CRYPT_OK)         { goto cleanup; }
      if ((err = mp_mod(U, t2N1, U)) != CRYPT_OK)                                { goto cleanup; }
      if ((err = mp_add(t2N1, U, q)) != CRYPT_OK)                                { goto cleanup; }
      if (!mp_isodd(q)) mp_add_d(q, 1, q);
      if ((err = mp_prime_is_prime(q, mr_tests_q, &res)) != CRYPT_OK)            { goto cleanup; }       /* XXX-TODO rounds are ignored; no Lucas test */
      if (res == LTC_MP_YES) found_q = 1;
    }

    /* p */
    if ((err = mp_read_unsigned_bin(seedinc, sbuf, seedbytes)) != CRYPT_OK)      { goto cleanup; }
    /* printf("seed="); mp_fwrite(seedinc, 16, stdout); printf("\n"); //XXX-DEBUG */
    if ((err = mp_add(q, q, t2q)) != CRYPT_OK)                                   { goto cleanup; }
    for(counter=0; counter < 4*L && !found_p; counter++) {
      for(j=0; j<=n; j++) {
        if ((err = mp_add_d(seedinc, 1, seedinc)) != CRYPT_OK)                   { goto cleanup; }
        if ((err = mp_mod(seedinc, t2seedlen, seedinc)) != CRYPT_OK)             { goto cleanup; }
        /* seedinc = (seedinc+1) % 2^seed_bitlen */
        if ((i = mp_unsigned_bin_size(seedinc)) > seedbytes)                     { err = CRYPT_INVALID_ARG; goto cleanup; }
        zeromem(sbuf, seedbytes);
        if ((err = mp_to_unsigned_bin(seedinc, sbuf + seedbytes-i)) != CRYPT_OK) { goto cleanup; }
        i = outbytes;
        err = hash_memory(hash, sbuf, seedbytes, wbuf+(n-j)*outbytes, &i);
        if (err != CRYPT_OK)                                                     { goto cleanup; }
      }
      if ((err = mp_read_unsigned_bin(W, wbuf, (n+1)*outbytes)) != CRYPT_OK)     { goto cleanup; }
      if ((err = mp_mod(W, t2L1, W)) != CRYPT_OK)                                { goto cleanup; }
      if ((err = mp_add(W, t2L1, X)) != CRYPT_OK)                                { goto cleanup; }
      if ((err = mp_mod(X, t2q, c))  != CRYPT_OK)                                { goto cleanup; }
      if ((err = mp_sub_d(c, 1, p))  != CRYPT_OK)                                { goto cleanup; }
      if ((err = mp_sub(X, p, p))    != CRYPT_OK)                                { goto cleanup; }
      if (mp_cmp(p, t2L1) != LTC_MP_LT) {
        /* p >= 2^(L-1) */
        if ((err = mp_prime_is_prime(p, mr_tests_p, &res)) != CRYPT_OK)          { goto cleanup; }       /* XXX-TODO rounds are ignored; no Lucas test */
        if (res == LTC_MP_YES) {
          found_p = 1;
        }
      }
    }
  }

 /* FIPS-186-4 A.2.1 Unverifiable Generation of the Generator g
  * 1. e = (p - 1)/q
  * 2. h = any integer satisfying: 1 < h < (p - 1)
  *    h could be obtained from a random number generator or from a counter that changes after each use
  * 3. g = h^e mod p
  * 4. if (g == 1), then go to step 2.
  *
  */

  if ((err = mp_sub_d(p, 1, e)) != CRYPT_OK)                                     { goto cleanup; }
  if ((err = mp_div(e, q, e, c)) != CRYPT_OK)                                    { goto cleanup; }
  /* e = (p - 1)/q */
  i = mp_count_bits(p);
  do {
    do {
      if ((err = rand_bn_bits(h, i, prng, wprng)) != CRYPT_OK)                   { goto cleanup; }
    } while (mp_cmp(h, p) != LTC_MP_LT || mp_cmp_d(h, 2) != LTC_MP_GT);
    if ((err = mp_sub_d(h, 1, h)) != CRYPT_OK)                                   { goto cleanup; }
    /* h is randon and 1 < h < (p-1) */
    if ((err = mp_exptmod(h, e, p, g)) != CRYPT_OK)                              { goto cleanup; }
  } while (mp_cmp_d(g, 1) == LTC_MP_EQ);

  err = CRYPT_OK;
cleanup:
  mp_clear_multi(t2L1, t2N1, t2q, t2seedlen, U, W, X, c, h, e, seedinc, NULL);
cleanup1:
  XFREE(sbuf);
cleanup2:
  XFREE(wbuf);
cleanup3:
  return err;
}

/**
  Create a DSA key (with given params)
  @param prng          An active PRNG state
  @param wprng         The index of the PRNG desired
  @param group_size    Size of the multiplicative group (octets)
  @param modulus_size  Size of the modulus (octets)
  @param key           [out] Where to store the created key
  @param p_hex         Hexadecimal string 'p'
  @param q_hex         Hexadecimal string 'q'
  @param g_hex         Hexadecimal string 'g'
  @return CRYPT_OK if successful, upon error this function will free all allocated memory
*/
int dsa_make_key_ex(prng_state *prng, int wprng, int group_size, int modulus_size, dsa_key *key, char* p_hex, char* q_hex, char* g_hex)
{
  int err, qbits;

  LTC_ARGCHK(key  != NULL);

  /* init mp_ints */
  if ((err = mp_init_multi(&key->g, &key->q, &key->p, &key->x, &key->y, NULL)) != CRYPT_OK) {
    return err;
  }

  if (p_hex == NULL || q_hex == NULL || g_hex == NULL) {
    /* generate params */
    err = dsa_make_params(prng, wprng, group_size, modulus_size, key->p, key->q, key->g);
    if (err != CRYPT_OK)                                                         { goto cleanup; }
  }
  else {
    /* read params */
    if ((err = mp_read_radix(key->p, p_hex, 16)) != CRYPT_OK)                    { goto cleanup; }
    if ((err = mp_read_radix(key->q, q_hex, 16)) != CRYPT_OK)                    { goto cleanup; }
    if ((err = mp_read_radix(key->g, g_hex, 16)) != CRYPT_OK)                    { goto cleanup; }
    /* XXX-TODO maybe do some validity check for p, q, g */
  }

  /* so now we have our DH structure, generator g, order q, modulus p
     Now we need a random exponent [mod q] and it's power g^x mod p
   */
  qbits = mp_count_bits(key->q);
  do {
     if ((err = rand_bn_bits(key->x, qbits, prng, wprng)) != CRYPT_OK)                  { goto cleanup; }
     /* private key x should be from range: 1 <= x <= q-1 (see FIPS 186-4 B.1.2) */
  } while (mp_cmp_d(key->x, 0) != LTC_MP_GT || mp_cmp(key->x, key->q) != LTC_MP_LT);
  if ((err = mp_exptmod(key->g, key->x, key->p, key->y)) != CRYPT_OK)                   { goto cleanup; }
  key->type = PK_PRIVATE;
  key->qord = group_size;

  return CRYPT_OK;

cleanup:
  mp_clear_multi(key->g, key->q, key->p, key->x, key->y, NULL);
  return err;
}

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
  return dsa_make_key_ex(prng, wprng, group_size, modulus_size, key, NULL, NULL, NULL);
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
