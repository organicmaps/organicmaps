/* test the ciphers and hashes using their built-in self-tests */

#include <tomcrypt_test.h>

int cipher_hash_test(void)
{
   int           x;
   unsigned char buf[4096];
   unsigned long n;
   prng_state    nprng;
   
   /* test ciphers */
   for (x = 0; cipher_descriptor[x].name != NULL; x++) {
      DO(cipher_descriptor[x].test());
   }
   
   /* test hashes */
   for (x = 0; hash_descriptor[x].name != NULL; x++) {
      DO(hash_descriptor[x].test());
   }
 
   /* test prngs (test, import/export */
   for (x = 0; prng_descriptor[x].name != NULL; x++) {
      DO(prng_descriptor[x].test());
      DO(prng_descriptor[x].start(&nprng));
      DO(prng_descriptor[x].add_entropy((unsigned char *)"helloworld12", 12, &nprng));
      DO(prng_descriptor[x].ready(&nprng));
      n = sizeof(buf);
      DO(prng_descriptor[x].pexport(buf, &n, &nprng));
      prng_descriptor[x].done(&nprng);
      DO(prng_descriptor[x].pimport(buf, n, &nprng));
      DO(prng_descriptor[x].ready(&nprng));
      if (prng_descriptor[x].read(buf, 100, &nprng) != 100) {
         fprintf(stderr, "Error reading from imported PRNG!\n");
         exit(EXIT_FAILURE);
      }
      prng_descriptor[x].done(&nprng);
   }
   
   return 0;
}

/* $Source: /cvs/libtom/libtomcrypt/testprof/cipher_hash_test.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2005/05/05 14:35:59 $ */
