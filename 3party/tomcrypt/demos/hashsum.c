/*
 * Written by Daniel Richards <kyhwana@world-net.co.nz> 6/7/2002
 * hash.c: This app uses libtomcrypt to hash either stdin or a file
 * This file is Public Domain. No rights are reserved.
 * Compile with 'gcc hashsum.c -o hashsum -ltomcrypt'
 * This example isn't really big enough to warrent splitting into
 * more functions ;)
*/

#include <tomcrypt.h>

int errno;

void register_algs();

int main(int argc, char **argv)
{
   int idx, x, z;
   unsigned long w;
   unsigned char hash_buffer[MAXBLOCKSIZE];
   hash_state md;

   /* You need to register algorithms before using them */
   register_algs();
   if (argc < 2) {
      printf("usage: ./hash algorithm file [file ...]\n");
      printf("Algorithms:\n");
      for (x = 0; hash_descriptor[x].name != NULL; x++) {
         printf(" %s (%d)\n", hash_descriptor[x].name, hash_descriptor[x].ID);
      }
      exit(EXIT_SUCCESS);
   }

   idx = find_hash(argv[1]);
   if (idx == -1) {
      fprintf(stderr, "\nInvalid hash specified on command line.\n");
      return -1;
   }

   if (argc == 2) {
      hash_descriptor[idx].init(&md);
      do {
         x = fread(hash_buffer, 1, sizeof(hash_buffer), stdin);
         hash_descriptor[idx].process(&md, hash_buffer, x);
      } while (x == sizeof(hash_buffer));
      hash_descriptor[idx].done(&md, hash_buffer);
      for (x = 0; x < (int)hash_descriptor[idx].hashsize; x++) {
          printf("%02x",hash_buffer[x]);
      }
      printf("  (stdin)\n");
   } else {
      for (z = 2; z < argc; z++) {
         w = sizeof(hash_buffer);
         if ((errno = hash_file(idx,argv[z],hash_buffer,&w)) != CRYPT_OK) {
            printf("File hash error: %s\n", error_to_string(errno));
         } else {
             for (x = 0; x < (int)hash_descriptor[idx].hashsize; x++) {
                 printf("%02x",hash_buffer[x]);
             }
             printf("  %s\n", argv[z]);
         }
      }
   }
   return EXIT_SUCCESS;
}

void register_algs(void)
{
  int err;

#ifdef LTC_TIGER
  register_hash (&tiger_desc);
#endif
#ifdef LTC_MD2
  register_hash (&md2_desc);
#endif
#ifdef LTC_MD4
  register_hash (&md4_desc);
#endif
#ifdef LTC_MD5
  register_hash (&md5_desc);
#endif
#ifdef LTC_SHA1
  register_hash (&sha1_desc);
#endif
#ifdef LTC_SHA224
  register_hash (&sha224_desc);
#endif
#ifdef LTC_SHA256
  register_hash (&sha256_desc);
#endif
#ifdef LTC_SHA384
  register_hash (&sha384_desc);
#endif
#ifdef LTC_SHA512
  register_hash (&sha512_desc);
#endif
#ifdef LTC_RIPEMD128
  register_hash (&rmd128_desc);
#endif
#ifdef LTC_RIPEMD160
  register_hash (&rmd160_desc);
#endif
#ifdef LTC_WHIRLPOOL
  register_hash (&whirlpool_desc);
#endif
#ifdef LTC_CHC_HASH
  register_hash(&chc_desc);
  if ((err = chc_register(register_cipher(&aes_enc_desc))) != CRYPT_OK) {
     printf("chc_register error: %s\n", error_to_string(err));
     exit(EXIT_FAILURE);
  }
#endif

}

/* $Source: /cvs/libtom/libtomcrypt/demos/hashsum.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2007/05/12 14:32:35 $ */
