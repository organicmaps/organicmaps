#include  <tomcrypt_test.h>

#ifdef LTC_BASE64
int base64_test(void)
{
   unsigned char in[64], out[256], tmp[64];
   unsigned long x, l1, l2, slen1;

   /*
    TEST CASES SOURCE:

    Network Working Group                                       S. Josefsson
    Request for Comments: 4648                                           SJD
    Obsoletes: 3548                                             October 2006
    Category: Standards Track
    */
   const struct {
     const char* s;
     const char* b64;
   } cases[] = {
       {"", ""              },
       {"f", "Zg=="         },
       {"fo", "Zm8="        },
       {"foo", "Zm9v"       },
       {"foob", "Zm9vYg=="  },
       {"fooba", "Zm9vYmE=" },
       {"foobar", "Zm9vYmFy"}
   };

   for (x = 0; x < sizeof(cases)/sizeof(cases[0]); ++x) {
       slen1 = strlen(cases[x].s);
       l1 = sizeof(out);
       DO(base64_encode((unsigned char*)cases[x].s, slen1, out, &l1));
       l2 = sizeof(tmp);
       DO(base64_decode(out, l1, tmp, &l2));
       if (l2 != slen1 || l1 != strlen(cases[x].b64) || memcmp(tmp, cases[x].s, l2) || memcmp(out, cases[x].b64, l1)) {
           fprintf(stderr, "\nbase64 failed case %lu", x);
           fprintf(stderr, "\nbase64 should: %s", cases[x].b64);
           out[sizeof(out)-1] = '\0';
           fprintf(stderr, "\nbase64 is:     %s", out);
           fprintf(stderr, "\nplain  should: %s", cases[x].s);
           tmp[sizeof(tmp)-1] = '\0';
           fprintf(stderr, "\nplain  is:     %s\n", tmp);
           return 1;
       }
   }

   for  (x = 0; x < 64; x++) {
       yarrow_read(in, x, &yarrow_prng);
       l1 = sizeof(out);
       DO(base64_encode(in, x, out, &l1));
       l2 = sizeof(tmp);
       DO(base64_decode(out, l1, tmp, &l2));
       if (l2 != x || memcmp(tmp, in, x)) {
           fprintf(stderr, "base64 failed %lu %lu %lu", x, l1, l2);
           return 1;
       }
   }
   return 0;
}
#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
