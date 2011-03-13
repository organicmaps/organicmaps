#include <tomcrypt_test.h>

/* Test store/load macros with offsets */
int store_test(void)
{
  unsigned char buf[256];
  int y;
  ulong32 L, L1;
  ulong64 LL, LL1;
#ifdef LTC_FAST
  int x, z;
#endif

  for (y = 0; y < 4; y++) {
      L  = 0x12345678UL;
      L1 = 0;
      STORE32L(L, buf + y);
      LOAD32L(L1, buf + y);
      if (L1 != L) {
         fprintf(stderr, "\n32L failed at offset %d\n", y);
         return 1;
      }
      STORE32H(L, buf + y);
      LOAD32H(L1, buf + y);
      if (L1 != L) {
         fprintf(stderr, "\n32H failed at offset %d\n", y);
         return 1;
      }
  }

  for (y = 0; y < 8; y++) {
      LL = CONST64 (0x01020304050607);
      LL1 = 0;
      STORE64L(LL, buf + y);
      LOAD64L(LL1, buf + y);
      if (LL1 != LL) {
         fprintf(stderr, "\n64L failed at offset %d\n", y);
         return 1;
      }
      STORE64H(LL, buf + y);
      LOAD64H(LL1, buf + y);
      if (LL1 != LL) {
         fprintf(stderr, "\n64H failed at offset %d\n", y);
         return 1;
      }
  }

/* test LTC_FAST */
#ifdef LTC_FAST
  y = 16;

  for (z = 0; z < y; z++) {
     /* fill y bytes with random */
     yarrow_read(buf+z,   y, &yarrow_prng);
     yarrow_read(buf+z+y, y, &yarrow_prng);

     /* now XOR it byte for byte */
     for (x = 0; x < y; x++) {
         buf[2*y+z+x] = buf[z+x] ^ buf[z+y+x];
     }

     /* now XOR it word for word */
     for (x = 0; x < y; x += sizeof(LTC_FAST_TYPE)) {
        *((LTC_FAST_TYPE*)(&buf[3*y+z+x])) = *((LTC_FAST_TYPE*)(&buf[z+x])) ^ *((LTC_FAST_TYPE*)(&buf[z+y+x]));
     }

     if (memcmp(&buf[2*y+z], &buf[3*y+z], y)) {
        fprintf(stderr, "\nLTC_FAST failed at offset %d\n", z);
        return 1;
     }
  }
#endif
  return 0;
}

/* $Source: /cvs/libtom/libtomcrypt/testprof/store_test.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2005/05/05 14:35:59 $ */
