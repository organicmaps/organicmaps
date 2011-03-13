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
  @file rsa_import.c
  Import a LTC_PKCS RSA key, Tom St Denis
*/  

#ifdef LTC_MRSA

/**
  Import an RSAPublicKey or RSAPrivateKey [two-prime only, only support >= 1024-bit keys, defined in LTC_PKCS #1 v2.1]
  @param in      The packet to import from
  @param inlen   It's length (octets)
  @param key     [out] Destination for newly imported key
  @return CRYPT_OK if successful, upon error allocated memory is freed
*/
int rsa_import(const unsigned char *in, unsigned long inlen, rsa_key *key)
{
   int           err;
   void         *zero;
   unsigned char *tmpbuf;
   unsigned long  t, x, y, z, tmpoid[16];
   ltc_asn1_list ssl_pubkey_hashoid[2];
   ltc_asn1_list ssl_pubkey[2];

   LTC_ARGCHK(in          != NULL);
   LTC_ARGCHK(key         != NULL);
   LTC_ARGCHK(ltc_mp.name != NULL);

   /* init key */
   if ((err = mp_init_multi(&key->e, &key->d, &key->N, &key->dQ, 
                            &key->dP, &key->qP, &key->p, &key->q, NULL)) != CRYPT_OK) {
      return err;
   }

   /* see if the OpenSSL DER format RSA public key will work */
   tmpbuf = XCALLOC(1, MAX_RSA_SIZE*8);
   if (tmpbuf == NULL) {
       err = CRYPT_MEM;
       goto LBL_ERR;
   }

   /* this includes the internal hash ID and optional params (NULL in this case) */
   LTC_SET_ASN1(ssl_pubkey_hashoid, 0, LTC_ASN1_OBJECT_IDENTIFIER, tmpoid,                sizeof(tmpoid)/sizeof(tmpoid[0]));   
   LTC_SET_ASN1(ssl_pubkey_hashoid, 1, LTC_ASN1_NULL,              NULL,                  0);

   /* the actual format of the SSL DER key is odd, it stores a RSAPublicKey in a **BIT** string ... so we have to extract it
      then proceed to convert bit to octet 
    */
   LTC_SET_ASN1(ssl_pubkey, 0,         LTC_ASN1_SEQUENCE,          &ssl_pubkey_hashoid,   2);
   LTC_SET_ASN1(ssl_pubkey, 1,         LTC_ASN1_BIT_STRING,        tmpbuf,                MAX_RSA_SIZE*8);

   if (der_decode_sequence(in, inlen,
                           ssl_pubkey, 2UL) == CRYPT_OK) {

      /* ok now we have to reassemble the BIT STRING to an OCTET STRING.  Thanks OpenSSL... */
      for (t = y = z = x = 0; x < ssl_pubkey[1].size; x++) {
          y = (y << 1) | tmpbuf[x];
          if (++z == 8) {
             tmpbuf[t++] = (unsigned char)y;
             y           = 0;
             z           = 0;
          }
      }

      /* now it should be SEQUENCE { INTEGER, INTEGER } */
      if ((err = der_decode_sequence_multi(tmpbuf, t,
                                           LTC_ASN1_INTEGER, 1UL, key->N, 
                                           LTC_ASN1_INTEGER, 1UL, key->e, 
                                           LTC_ASN1_EOL,     0UL, NULL)) != CRYPT_OK) {
         XFREE(tmpbuf);
         goto LBL_ERR;
      }
      XFREE(tmpbuf);
      key->type = PK_PUBLIC;
      return CRYPT_OK;
   }
   XFREE(tmpbuf);

   /* not SSL public key, try to match against LTC_PKCS #1 standards */
   if ((err = der_decode_sequence_multi(in, inlen, 
                                  LTC_ASN1_INTEGER, 1UL, key->N, 
                                  LTC_ASN1_EOL,     0UL, NULL)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   if (mp_cmp_d(key->N, 0) == LTC_MP_EQ) {
      if ((err = mp_init(&zero)) != CRYPT_OK) { 
         goto LBL_ERR;
      }
      /* it's a private key */
      if ((err = der_decode_sequence_multi(in, inlen, 
                          LTC_ASN1_INTEGER, 1UL, zero, 
                          LTC_ASN1_INTEGER, 1UL, key->N, 
                          LTC_ASN1_INTEGER, 1UL, key->e,
                          LTC_ASN1_INTEGER, 1UL, key->d, 
                          LTC_ASN1_INTEGER, 1UL, key->p, 
                          LTC_ASN1_INTEGER, 1UL, key->q, 
                          LTC_ASN1_INTEGER, 1UL, key->dP,
                          LTC_ASN1_INTEGER, 1UL, key->dQ, 
                          LTC_ASN1_INTEGER, 1UL, key->qP, 
                          LTC_ASN1_EOL,     0UL, NULL)) != CRYPT_OK) {
         mp_clear(zero);
         goto LBL_ERR;
      }
      mp_clear(zero);
      key->type = PK_PRIVATE;
   } else if (mp_cmp_d(key->N, 1) == LTC_MP_EQ) {
      /* we don't support multi-prime RSA */
      err = CRYPT_PK_INVALID_TYPE;
      goto LBL_ERR;
   } else {
      /* it's a public key and we lack e */
      if ((err = der_decode_sequence_multi(in, inlen, 
                                     LTC_ASN1_INTEGER, 1UL, key->N, 
                                     LTC_ASN1_INTEGER, 1UL, key->e, 
                                     LTC_ASN1_EOL,     0UL, NULL)) != CRYPT_OK) {
         goto LBL_ERR;
      }
      key->type = PK_PUBLIC;
   }
   return CRYPT_OK;
LBL_ERR:
   mp_clear_multi(key->d,  key->e, key->N, key->dQ, key->dP, key->qP, key->p, key->q, NULL);
   return err;
}

#endif /* LTC_MRSA */


/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_import.c,v $ */
/* $Revision: 1.23 $ */
/* $Date: 2007/05/12 14:32:35 $ */
