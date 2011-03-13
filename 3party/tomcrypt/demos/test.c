#include <tomcrypt_test.h>

int main(void)
{
   int x;
   reg_algs();

#ifdef USE_LTM
   ltc_mp = ltm_desc;
#elif defined(USE_TFM)
   ltc_mp = tfm_desc;
#elif defined(USE_GMP)
   ltc_mp = gmp_desc;
#else
   extern ltc_math_descriptor EXT_MATH_LIB;
   ltc_mp = EXT_MATH_LIB;
#endif

   printf("build == \n%s\n", crypt_build_settings);
   printf("\nstore_test...."); fflush(stdout); x = store_test();       printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\ncipher_test..."); fflush(stdout); x = cipher_hash_test(); printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\nmodes_test...."); fflush(stdout); x = modes_test();       printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\nder_test......"); fflush(stdout); x = der_tests();        printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\nmac_test......"); fflush(stdout); x = mac_test();         printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\npkcs_1_test..."); fflush(stdout); x = pkcs_1_test();      printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\nrsa_test......"); fflush(stdout); x = rsa_test();         printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\necc_test......"); fflush(stdout); x = ecc_tests();        printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE); 
   printf("\ndsa_test......"); fflush(stdout); x = dsa_test();         printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\nkatja_test...."); fflush(stdout); x = katja_test();       printf(x ? "failed" : "passed");if (x) exit(EXIT_FAILURE);
   printf("\n");
   return EXIT_SUCCESS;
}

/* $Source: /cvs/libtom/libtomcrypt/demos/test.c,v $ */
/* $Revision: 1.28 $ */
/* $Date: 2006/05/25 10:50:08 $ */
