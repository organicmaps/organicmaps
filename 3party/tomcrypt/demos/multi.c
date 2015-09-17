/* test the multi helpers... */
#include <tomcrypt.h>

int main(void)
{
   unsigned char key[16], buf[2][MAXBLOCKSIZE];
   unsigned long len, len2;


/* register algos */
   register_hash(&sha256_desc);
   register_cipher(&aes_desc);

/* HASH testing */
   len = sizeof(buf[0]);
   hash_memory(find_hash("sha256"), (unsigned char*)"hello", 5, buf[0], &len);
   len2 = sizeof(buf[0]);
   hash_memory_multi(find_hash("sha256"), buf[1], &len2, (unsigned char*)"hello", 5, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }
   len2 = sizeof(buf[0]);
   hash_memory_multi(find_hash("sha256"), buf[1], &len2, (unsigned char*)"he", 2UL, "llo", 3UL, NULL, 0);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }
   len2 = sizeof(buf[0]);
   hash_memory_multi(find_hash("sha256"), buf[1], &len2, (unsigned char*)"h", 1UL, "e", 1UL, "l", 1UL, "l", 1UL, "o", 1UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }

/* LTC_HMAC */
   len = sizeof(buf[0]);
   hmac_memory(find_hash("sha256"), key, 16, (unsigned char*)"hello", 5, buf[0], &len);
   len2 = sizeof(buf[0]);
   hmac_memory_multi(find_hash("sha256"), key, 16, buf[1], &len2, (unsigned char*)"hello", 5UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }
   len2 = sizeof(buf[0]);
   hmac_memory_multi(find_hash("sha256"), key, 16, buf[1], &len2, (unsigned char*)"he", 2UL, "llo", 3UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }
   len2 = sizeof(buf[0]);
   hmac_memory_multi(find_hash("sha256"), key, 16, buf[1], &len2, (unsigned char*)"h", 1UL, "e", 1UL, "l", 1UL, "l", 1UL, "o", 1UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }

/* LTC_OMAC */
   len = sizeof(buf[0]);
   omac_memory(find_cipher("aes"), key, 16, (unsigned char*)"hello", 5, buf[0], &len);
   len2 = sizeof(buf[0]);
   omac_memory_multi(find_cipher("aes"), key, 16, buf[1], &len2, (unsigned char*)"hello", 5UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }
   len2 = sizeof(buf[0]);
   omac_memory_multi(find_cipher("aes"), key, 16, buf[1], &len2, (unsigned char*)"he", 2UL, "llo", 3UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }
   len2 = sizeof(buf[0]);
   omac_memory_multi(find_cipher("aes"), key, 16, buf[1], &len2, (unsigned char*)"h", 1UL, "e", 1UL, "l", 1UL, "l", 1UL, "o", 1UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }

/* PMAC */
   len = sizeof(buf[0]);
   pmac_memory(find_cipher("aes"), key, 16, (unsigned char*)"hello", 5, buf[0], &len);
   len2 = sizeof(buf[0]);
   pmac_memory_multi(find_cipher("aes"), key, 16, buf[1], &len2, (unsigned char*)"hello", 5, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }
   len2 = sizeof(buf[0]);
   pmac_memory_multi(find_cipher("aes"), key, 16, buf[1], &len2, (unsigned char*)"he", 2UL, "llo", 3UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }
   len2 = sizeof(buf[0]);
   pmac_memory_multi(find_cipher("aes"), key, 16, buf[1], &len2, (unsigned char*)"h", 1UL, "e", 1UL, "l", 1UL, "l", 1UL, "o", 1UL, NULL);
   if (len != len2 || memcmp(buf[0], buf[1], len)) {
      printf("Failed: %d %lu %lu\n", __LINE__, len, len2);
      return EXIT_FAILURE;
   }


   printf("All passed\n");
   return EXIT_SUCCESS;
}


/* $Source$ */
/* $Revision$ */
/* $Date$ */
