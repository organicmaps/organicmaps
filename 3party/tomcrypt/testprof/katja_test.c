#include <tomcrypt_test.h>

#ifdef MKAT

int katja_test(void)
{
   unsigned char in[1024], out[1024], tmp[1024];
   katja_key     key, privKey, pubKey;
   int           hash_idx, prng_idx, stat, stat2, size;
   unsigned long kat_msgsize, len, len2, cnt;
   static unsigned char lparam[] = { 0x01, 0x02, 0x03, 0x04 };

   hash_idx = find_hash("sha1");
   prng_idx = find_prng("yarrow");
   if (hash_idx == -1 || prng_idx == -1) {
      fprintf(stderr, "katja_test requires LTC_SHA1 and yarrow");
      return 1;
   }

for (size = 1024; size <= 2048; size += 256) {
  
   /* make 10 random key */
   for (cnt = 0; cnt < 10; cnt++) {
      DO(katja_make_key(&yarrow_prng, prng_idx, size/8, &key));
      if (mp_count_bits(key.N) < size - 7) {
         fprintf(stderr, "katja_%d key modulus has %d bits\n", size, mp_count_bits(key.N));

len = mp_unsigned_bin_size(key.N);
mp_to_unsigned_bin(key.N, tmp);
 fprintf(stderr, "N == \n");
for (cnt = 0; cnt < len; ) {
   fprintf(stderr, "%02x ", tmp[cnt]);
   if (!(++cnt & 15)) fprintf(stderr, "\n");
}

len = mp_unsigned_bin_size(key.p);
mp_to_unsigned_bin(key.p, tmp);
 fprintf(stderr, "p == \n");
for (cnt = 0; cnt < len; ) {
   fprintf(stderr, "%02x ", tmp[cnt]);
   if (!(++cnt & 15)) fprintf(stderr, "\n");
}

len = mp_unsigned_bin_size(key.q);
mp_to_unsigned_bin(key.q, tmp);
 fprintf(stderr, "\nq == \n");
for (cnt = 0; cnt < len; ) {
   fprintf(stderr, "%02x ", tmp[cnt]);
   if (!(++cnt & 15)) fprintf(stderr, "\n");
}
 fprintf(stderr, "\n");


         return 1;
      }
      if (cnt != 9) {
         katja_free(&key);
      }
   }
   /* encrypt the key (without lparam) */
   for (cnt = 0; cnt < 4; cnt++) {
   for (kat_msgsize = 1; kat_msgsize <= 42; kat_msgsize++) {
      /* make a random key/msg */
      yarrow_read(in, kat_msgsize, &yarrow_prng);

      len  = sizeof(out);
      len2 = kat_msgsize;
   
      DO(katja_encrypt_key(in, kat_msgsize, out, &len, NULL, 0, &yarrow_prng, prng_idx, hash_idx, &key));
      /* change a byte */
      out[8] ^= 1;
      DO(katja_decrypt_key(out, len, tmp, &len2, NULL, 0, hash_idx, &stat2, &key));
      /* change a byte back */
      out[8] ^= 1;
      if (len2 != kat_msgsize) {
         fprintf(stderr, "\nkatja_decrypt_key mismatch len %lu (first decrypt)", len2);
         return 1;
      }

      len2 = kat_msgsize;
      DO(katja_decrypt_key(out, len, tmp, &len2, NULL, 0, hash_idx, &stat, &key));
      if (!(stat == 1 && stat2 == 0)) {
         fprintf(stderr, "katja_decrypt_key failed");
         return 1;
      }
      if (len2 != kat_msgsize || memcmp(tmp, in, kat_msgsize)) {
         unsigned long x;
         fprintf(stderr, "\nkatja_decrypt_key mismatch, len %lu (second decrypt)\n", len2);
         fprintf(stderr, "Original contents: \n"); 
         for (x = 0; x < kat_msgsize; ) {
             fprintf(stderr, "%02x ", in[x]);
             if (!(++x % 16)) {
                fprintf(stderr, "\n");
             }
         }
         fprintf(stderr, "\n");
         fprintf(stderr, "Output contents: \n"); 
         for (x = 0; x < kat_msgsize; ) {
             fprintf(stderr, "%02x ", out[x]);
             if (!(++x % 16)) {
                fprintf(stderr, "\n");
             }
         }     
         fprintf(stderr, "\n");
         return 1;
      }
   }
   }

   /* encrypt the key (with lparam) */
   for (kat_msgsize = 1; kat_msgsize <= 42; kat_msgsize++) {
      len  = sizeof(out);
      len2 = kat_msgsize;
      DO(katja_encrypt_key(in, kat_msgsize, out, &len, lparam, sizeof(lparam), &yarrow_prng, prng_idx, hash_idx, &key));
      /* change a byte */
      out[8] ^= 1;
      DO(katja_decrypt_key(out, len, tmp, &len2, lparam, sizeof(lparam), hash_idx, &stat2, &key));
      if (len2 != kat_msgsize) {
         fprintf(stderr, "\nkatja_decrypt_key mismatch len %lu (first decrypt)", len2);
         return 1;
      }
      /* change a byte back */
      out[8] ^= 1;

      len2 = kat_msgsize;
      DO(katja_decrypt_key(out, len, tmp, &len2, lparam, sizeof(lparam), hash_idx, &stat, &key));
      if (!(stat == 1 && stat2 == 0)) {
         fprintf(stderr, "katja_decrypt_key failed");
         return 1;
      }
      if (len2 != kat_msgsize || memcmp(tmp, in, kat_msgsize)) {
         fprintf(stderr, "katja_decrypt_key mismatch len %lu", len2);
         return 1;
      }
   }

#if 0

   /* sign a message (unsalted, lower cholestorol and Atkins approved) now */
   len = sizeof(out);
   DO(katja_sign_hash(in, 20, out, &len, &yarrow_prng, prng_idx, hash_idx, 0, &key));

/* export key and import as both private and public */
   len2 = sizeof(tmp);
   DO(katja_export(tmp, &len2, PK_PRIVATE, &key)); 
   DO(katja_import(tmp, len2, &privKey)); 
   len2 = sizeof(tmp);
   DO(katja_export(tmp, &len2, PK_PUBLIC, &key));
   DO(katja_import(tmp, len2, &pubKey));

   /* verify with original */
   DO(katja_verify_hash(out, len, in, 20, hash_idx, 0, &stat, &key));
   /* change a byte */
   in[0] ^= 1;
   DO(katja_verify_hash(out, len, in, 20, hash_idx, 0, &stat2, &key));
   
   if (!(stat == 1 && stat2 == 0)) {
      fprintf(stderr, "katja_verify_hash (unsalted, origKey) failed, %d, %d", stat, stat2);
      katja_free(&key);
      katja_free(&pubKey);
      katja_free(&privKey);
      return 1;
   }

   /* verify with privKey */
   /* change a byte */
   in[0] ^= 1;
   DO(katja_verify_hash(out, len, in, 20, hash_idx, 0, &stat, &privKey));
   /* change a byte */
   in[0] ^= 1;
   DO(katja_verify_hash(out, len, in, 20, hash_idx, 0, &stat2, &privKey));
   
   if (!(stat == 1 && stat2 == 0)) {
      fprintf(stderr, "katja_verify_hash (unsalted, privKey) failed, %d, %d", stat, stat2);
      katja_free(&key);
      katja_free(&pubKey);
      katja_free(&privKey);
      return 1;
   }

   /* verify with pubKey */
   /* change a byte */
   in[0] ^= 1;
   DO(katja_verify_hash(out, len, in, 20, hash_idx, 0, &stat, &pubKey));
   /* change a byte */
   in[0] ^= 1;
   DO(katja_verify_hash(out, len, in, 20, hash_idx, 0, &stat2, &pubKey));
   
   if (!(stat == 1 && stat2 == 0)) {
      fprintf(stderr, "katja_verify_hash (unsalted, pubkey) failed, %d, %d", stat, stat2);
      katja_free(&key);
      katja_free(&pubKey);
      katja_free(&privKey);
      return 1;
   }

   /* sign a message (salted) now (use privKey to make, pubKey to verify) */
   len = sizeof(out);
   DO(katja_sign_hash(in, 20, out, &len, &yarrow_prng, prng_idx, hash_idx, 8, &privKey));
   DO(katja_verify_hash(out, len, in, 20, hash_idx, 8, &stat, &pubKey));
   /* change a byte */
   in[0] ^= 1;
   DO(katja_verify_hash(out, len, in, 20, hash_idx, 8, &stat2, &pubKey));
   
   if (!(stat == 1 && stat2 == 0)) {
      fprintf(stderr, "katja_verify_hash (salted) failed, %d, %d", stat, stat2);
      katja_free(&key);
      katja_free(&pubKey);
      katja_free(&privKey);
      return 1;
   }
#endif

   katja_free(&key);
   katja_free(&pubKey);
   katja_free(&privKey);
}
   
   /* free the key and return */
   return 0;
}

#else

int katja_test(void)
{
   fprintf(stderr, "NOP");
   return 0;
}

#endif
