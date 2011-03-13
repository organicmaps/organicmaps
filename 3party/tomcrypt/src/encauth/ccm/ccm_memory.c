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
  @file ccm_memory.c
  CCM support, process a block of memory, Tom St Denis
*/

#ifdef LTC_CCM_MODE

/**
   CCM encrypt/decrypt and produce an authentication tag
   @param cipher     The index of the cipher desired
   @param key        The secret key to use
   @param keylen     The length of the secret key (octets)
   @param uskey      A previously scheduled key [optional can be NULL]
   @param nonce      The session nonce [use once]
   @param noncelen   The length of the nonce
   @param header     The header for the session
   @param headerlen  The length of the header (octets)
   @param pt         [out] The plaintext
   @param ptlen      The length of the plaintext (octets)
   @param ct         [out] The ciphertext
   @param tag        [out] The destination tag
   @param taglen     [in/out] The max size and resulting size of the authentication tag
   @param direction  Encrypt or Decrypt direction (0 or 1)
   @return CRYPT_OK if successful
*/
int ccm_memory(int cipher,
    const unsigned char *key,    unsigned long keylen,
    symmetric_key       *uskey,
    const unsigned char *nonce,  unsigned long noncelen,
    const unsigned char *header, unsigned long headerlen,
          unsigned char *pt,     unsigned long ptlen,
          unsigned char *ct,
          unsigned char *tag,    unsigned long *taglen,
                    int  direction)
{
   unsigned char  PAD[16], ctr[16], CTRPAD[16], b;
   symmetric_key *skey;
   int            err;
   unsigned long  len, L, x, y, z, CTRlen;

   if (uskey == NULL) {
      LTC_ARGCHK(key    != NULL);
   }
   LTC_ARGCHK(nonce  != NULL);
   if (headerlen > 0) {
      LTC_ARGCHK(header != NULL);
   }
   LTC_ARGCHK(pt     != NULL);
   LTC_ARGCHK(ct     != NULL);
   LTC_ARGCHK(tag    != NULL);
   LTC_ARGCHK(taglen != NULL);

#ifdef LTC_FAST
   if (16 % sizeof(LTC_FAST_TYPE)) {
      return CRYPT_INVALID_ARG;
   }
#endif

   /* check cipher input */
   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }
   if (cipher_descriptor[cipher].block_length != 16) {
      return CRYPT_INVALID_CIPHER;
   }

   /* make sure the taglen is even and <= 16 */
   *taglen &= ~1;
   if (*taglen > 16) {
      *taglen = 16;
   }

   /* can't use < 4 */
   if (*taglen < 4) {
      return CRYPT_INVALID_ARG;
   }

   /* is there an accelerator? */
   if (cipher_descriptor[cipher].accel_ccm_memory != NULL) {
       return cipher_descriptor[cipher].accel_ccm_memory(
           key,    keylen,
           uskey,
           nonce,  noncelen,
           header, headerlen,
           pt,     ptlen,
           ct, 
           tag,    taglen,
           direction);
   }

   /* let's get the L value */
   len = ptlen;
   L   = 0;
   while (len) {
      ++L;
      len >>= 8;
   }
   if (L <= 1) {
      L = 2;
   }

   /* increase L to match the nonce len */
   noncelen = (noncelen > 13) ? 13 : noncelen;
   if ((15 - noncelen) > L) {
      L = 15 - noncelen;
   }

   /* decrease noncelen to match L */
   if ((noncelen + L) > 15) {
      noncelen = 15 - L;
   }

   /* allocate mem for the symmetric key */
   if (uskey == NULL) {
      skey = XMALLOC(sizeof(*skey));
      if (skey == NULL) {
         return CRYPT_MEM;
      }

      /* initialize the cipher */
      if ((err = cipher_descriptor[cipher].setup(key, keylen, 0, skey)) != CRYPT_OK) {
         XFREE(skey);
         return err;
      }
   } else {
      skey = uskey;
   }

   /* form B_0 == flags | Nonce N | l(m) */
   x = 0;
   PAD[x++] = (unsigned char)(((headerlen > 0) ? (1<<6) : 0) |
            (((*taglen - 2)>>1)<<3)        |
            (L-1));

   /* nonce */
   for (y = 0; y < (16 - (L + 1)); y++) {
       PAD[x++] = nonce[y];
   }

   /* store len */
   len = ptlen;

   /* shift len so the upper bytes of len are the contents of the length */
   for (y = L; y < 4; y++) {
       len <<= 8;
   }

   /* store l(m) (only store 32-bits) */
   for (y = 0; L > 4 && (L-y)>4; y++) {
       PAD[x++] = 0;
   }
   for (; y < L; y++) {
       PAD[x++] = (unsigned char)((len >> 24) & 255);
       len <<= 8;
   }

   /* encrypt PAD */
   if ((err = cipher_descriptor[cipher].ecb_encrypt(PAD, PAD, skey)) != CRYPT_OK) {
       goto error;
   }

   /* handle header */
   if (headerlen > 0) {
      x = 0;
      
      /* store length */
      if (headerlen < ((1UL<<16) - (1UL<<8))) {
         PAD[x++] ^= (headerlen>>8) & 255;
         PAD[x++] ^= headerlen & 255;
      } else {
         PAD[x++] ^= 0xFF;
         PAD[x++] ^= 0xFE;
         PAD[x++] ^= (headerlen>>24) & 255;
         PAD[x++] ^= (headerlen>>16) & 255;
         PAD[x++] ^= (headerlen>>8) & 255;
         PAD[x++] ^= headerlen & 255;
      }

      /* now add the data */
      for (y = 0; y < headerlen; y++) {
          if (x == 16) {
             /* full block so let's encrypt it */
             if ((err = cipher_descriptor[cipher].ecb_encrypt(PAD, PAD, skey)) != CRYPT_OK) {
                goto error;
             }
             x = 0;
          }
          PAD[x++] ^= header[y];
      }

      /* remainder? */
      if (x != 0) {
         if ((err = cipher_descriptor[cipher].ecb_encrypt(PAD, PAD, skey)) != CRYPT_OK) {
            goto error;
         }
      }
   }

   /* setup the ctr counter */
   x = 0;

   /* flags */
   ctr[x++] = (unsigned char)L-1;
 
   /* nonce */
   for (y = 0; y < (16 - (L+1)); ++y) {
      ctr[x++] = nonce[y];
   }
   /* offset */
   while (x < 16) {
      ctr[x++] = 0;
   }

   x      = 0;
   CTRlen = 16;

   /* now handle the PT */
   if (ptlen > 0) {
      y = 0;
#ifdef LTC_FAST
      if (ptlen & ~15)  {
          if (direction == CCM_ENCRYPT) {
             for (; y < (ptlen & ~15); y += 16) {
                /* increment the ctr? */
                for (z = 15; z > 15-L; z--) {
                    ctr[z] = (ctr[z] + 1) & 255;
                    if (ctr[z]) break;
                }
                if ((err = cipher_descriptor[cipher].ecb_encrypt(ctr, CTRPAD, skey)) != CRYPT_OK) {
                   goto error;
                }

                /* xor the PT against the pad first */
                for (z = 0; z < 16; z += sizeof(LTC_FAST_TYPE)) {
                    *((LTC_FAST_TYPE*)(&PAD[z]))  ^= *((LTC_FAST_TYPE*)(&pt[y+z]));
                    *((LTC_FAST_TYPE*)(&ct[y+z])) = *((LTC_FAST_TYPE*)(&pt[y+z])) ^ *((LTC_FAST_TYPE*)(&CTRPAD[z]));
                }
                if ((err = cipher_descriptor[cipher].ecb_encrypt(PAD, PAD, skey)) != CRYPT_OK) {
                   goto error;
                }
             }
         } else {
             for (; y < (ptlen & ~15); y += 16) {
                /* increment the ctr? */
                for (z = 15; z > 15-L; z--) {
                    ctr[z] = (ctr[z] + 1) & 255;
                    if (ctr[z]) break;
                }
                if ((err = cipher_descriptor[cipher].ecb_encrypt(ctr, CTRPAD, skey)) != CRYPT_OK) {
                   goto error;
                }

                /* xor the PT against the pad last */
                for (z = 0; z < 16; z += sizeof(LTC_FAST_TYPE)) {
                    *((LTC_FAST_TYPE*)(&pt[y+z])) = *((LTC_FAST_TYPE*)(&ct[y+z])) ^ *((LTC_FAST_TYPE*)(&CTRPAD[z]));
                    *((LTC_FAST_TYPE*)(&PAD[z]))  ^= *((LTC_FAST_TYPE*)(&pt[y+z]));
                }
                if ((err = cipher_descriptor[cipher].ecb_encrypt(PAD, PAD, skey)) != CRYPT_OK) {
                   goto error;
                }
             }
         }
     }
#endif

      for (; y < ptlen; y++) {
          /* increment the ctr? */
          if (CTRlen == 16) {
             for (z = 15; z > 15-L; z--) {
                 ctr[z] = (ctr[z] + 1) & 255;
                 if (ctr[z]) break;
             }
             if ((err = cipher_descriptor[cipher].ecb_encrypt(ctr, CTRPAD, skey)) != CRYPT_OK) {
                goto error;
             }
             CTRlen = 0;
          }

          /* if we encrypt we add the bytes to the MAC first */
          if (direction == CCM_ENCRYPT) {
             b     = pt[y];
             ct[y] = b ^ CTRPAD[CTRlen++];
          } else {
             b     = ct[y] ^ CTRPAD[CTRlen++];
             pt[y] = b;
          }

          if (x == 16) {
             if ((err = cipher_descriptor[cipher].ecb_encrypt(PAD, PAD, skey)) != CRYPT_OK) {
                goto error;
             }
             x = 0;
          }
          PAD[x++] ^= b;
      }
             
      if (x != 0) {
         if ((err = cipher_descriptor[cipher].ecb_encrypt(PAD, PAD, skey)) != CRYPT_OK) {
            goto error;
         }
      }
   }

   /* setup CTR for the TAG (zero the count) */
   for (y = 15; y > 15 - L; y--) {
      ctr[y] = 0x00;
   }
   if ((err = cipher_descriptor[cipher].ecb_encrypt(ctr, CTRPAD, skey)) != CRYPT_OK) {
      goto error;
   }

   if (skey != uskey) {
      cipher_descriptor[cipher].done(skey);
   }

   /* store the TAG */
   for (x = 0; x < 16 && x < *taglen; x++) {
       tag[x] = PAD[x] ^ CTRPAD[x];
   }
   *taglen = x;

#ifdef LTC_CLEAN_STACK
   zeromem(skey,   sizeof(*skey));
   zeromem(PAD,    sizeof(PAD));
   zeromem(CTRPAD, sizeof(CTRPAD));
#endif
error:
   if (skey != uskey) {
      XFREE(skey);
   }

   return err;
}

#endif

/* $Source: /cvs/libtom/libtomcrypt/src/encauth/ccm/ccm_memory.c,v $ */
/* $Revision: 1.20 $ */
/* $Date: 2007/05/12 14:32:35 $ */
