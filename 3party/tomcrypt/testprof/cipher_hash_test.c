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
      DOX(cipher_descriptor[x].test(), cipher_descriptor[x].name);
   }
   
   /* test hashes */
   for (x = 0; hash_descriptor[x].name != NULL; x++) {
      DOX(hash_descriptor[x].test(), hash_descriptor[x].name);
   }
 
   /* test prngs (test, import/export */
   for (x = 0; prng_descriptor[x].name != NULL; x++) {
      DOX(prng_descriptor[x].test(), prng_descriptor[x].name);
      DOX(prng_descriptor[x].start(&nprng), prng_descriptor[x].name);
      DOX(prng_descriptor[x].add_entropy((unsigned char *)"helloworld12", 12, &nprng), prng_descriptor[x].name);
      DOX(prng_descriptor[x].ready(&nprng), prng_descriptor[x].name);
      n = sizeof(buf);
      DOX(prng_descriptor[x].pexport(buf, &n, &nprng), prng_descriptor[x].name);
      prng_descriptor[x].done(&nprng);
      DOX(prng_descriptor[x].pimport(buf, n, &nprng), prng_descriptor[x].name);
      DOX(prng_descriptor[x].ready(&nprng), prng_descriptor[x].name);
      if (prng_descriptor[x].read(buf, 100, &nprng) != 100) {
         fprintf(stderr, "Error reading from imported PRNG!\n");
         exit(EXIT_FAILURE);
      }
      prng_descriptor[x].done(&nprng);
   }
   
   return 0;
}

/* $Source$ */
/* $Revision$ */
/* $Date$ */
