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
  @file dsa_decrypt_key.c
  DSA Crypto, Tom St Denis
*/  

#ifdef LTC_MDSA

/**
  Decrypt an DSA encrypted key
  @param in       The ciphertext
  @param inlen    The length of the ciphertext (octets)
  @param out      [out] The plaintext
  @param outlen   [in/out] The max size and resulting size of the plaintext
  @param key      The corresponding private DSA key
  @return CRYPT_OK if successful
*/
int dsa_decrypt_key(const unsigned char *in,  unsigned long  inlen,
                          unsigned char *out, unsigned long *outlen, 
                          dsa_key *key)
{
   unsigned char  *skey, *expt;
   void           *g_pub;
   unsigned long  x, y, hashOID[32];
   int            hash, err;
   ltc_asn1_list  decode[3];

   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(key    != NULL);

   /* right key type? */
   if (key->type != PK_PRIVATE) {
      return CRYPT_PK_NOT_PRIVATE;
   }
   
   /* decode to find out hash */
   LTC_SET_ASN1(decode, 0, LTC_ASN1_OBJECT_IDENTIFIER, hashOID, sizeof(hashOID)/sizeof(hashOID[0]));
 
   if ((err = der_decode_sequence(in, inlen, decode, 1)) != CRYPT_OK) {
      return err;
   }

   hash = find_hash_oid(hashOID, decode[0].size);                   
   if (hash_is_valid(hash) != CRYPT_OK) {
      return CRYPT_INVALID_PACKET;
   }

   /* we now have the hash! */
   
   if ((err = mp_init(&g_pub)) != CRYPT_OK) {
      return err;
   }

   /* allocate memory */
   expt   = XMALLOC(mp_unsigned_bin_size(key->p) + 1);
   skey   = XMALLOC(MAXBLOCKSIZE);
   if (expt == NULL || skey == NULL) {
      if (expt != NULL) {
         XFREE(expt);
      }
      if (skey != NULL) {
         XFREE(skey);
      }
      mp_clear(g_pub);
      return CRYPT_MEM;
   }
   
   LTC_SET_ASN1(decode, 1, LTC_ASN1_INTEGER,          g_pub,      1UL);
   LTC_SET_ASN1(decode, 2, LTC_ASN1_OCTET_STRING,      skey,      MAXBLOCKSIZE);

   /* read the structure in now */
   if ((err = der_decode_sequence(in, inlen, decode, 3)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   /* make shared key */
   x = mp_unsigned_bin_size(key->p) + 1;
   if ((err = dsa_shared_secret(key->x, g_pub, key, expt, &x)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   y = MIN(mp_unsigned_bin_size(key->p) + 1, MAXBLOCKSIZE);
   if ((err = hash_memory(hash, expt, x, expt, &y)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   /* ensure the hash of the shared secret is at least as big as the encrypt itself */
   if (decode[2].size > y) {
      err = CRYPT_INVALID_PACKET;
      goto LBL_ERR;
   }

   /* avoid buffer overflow */
   if (*outlen < decode[2].size) {
      *outlen = decode[2].size;
      err = CRYPT_BUFFER_OVERFLOW;
      goto LBL_ERR;
   }

   /* Decrypt the key */
   for (x = 0; x < decode[2].size; x++) {
     out[x] = expt[x] ^ skey[x];
   }
   *outlen = x;

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(expt,   mp_unsigned_bin_size(key->p) + 1);
   zeromem(skey,   MAXBLOCKSIZE);
#endif

   XFREE(expt);
   XFREE(skey);
  
   mp_clear(g_pub);

   return err;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/pk/dsa/dsa_decrypt_key.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */

