#include <stdio.h>

unsigned E[16] =  { 1, 0xb, 9, 0xc, 0xd, 6, 0xf, 3, 0xe, 8, 7, 4, 0xa, 2, 5, 0 };
unsigned Ei[16];
unsigned R[16] =  { 7, 0xc, 0xb, 0xd, 0xe, 4, 9, 0xf, 6, 3, 8, 0xa, 2, 5, 1, 0 };
unsigned cir[8][8] = { 
 {1, 1, 4, 1, 8, 5, 2, 9 },
}; 


unsigned gf_mul(unsigned a, unsigned b)
{
   unsigned r;
   
   r = 0;
   while (a) {
      if (a & 1) r ^= b;
      a >>= 1;
      b = (b << 1) ^ (b & 0x80 ? 0x11d : 0x00);
   }
   return r;
}

unsigned sbox(unsigned x)
{
   unsigned a, b, w;
   
   a = x >> 4;
   b = x & 15;
   
   a = E[a]; b = Ei[b];
   w = a ^ b; w = R[w];
   a = E[a ^ w]; b = Ei[b ^ w];
   
   
   return (a << 4) | b;
}

int main(void)
{
   unsigned x, y;
   
   for (x = 0; x < 16; x++) Ei[E[x]] = x;
   
//   for (x = 0; x < 16; x++) printf("%2x ", sbox(x));
   for (y = 1; y < 8; y++) {
      for (x = 0; x < 8; x++) {
          cir[y][x] = cir[y-1][(x-1)&7];
      }
   }

/*   
   printf("\n");
   for (y = 0; y < 8; y++) {
       for (x = 0; x < 8; x++) printf("%2d ", cir[y][x]);
       printf("\n");
   }
*/

   for (y = 0; y < 8; y++) {
       printf("static const ulong64 sbox%d[] = {\n", y);
       for (x = 0; x < 256; ) {
           printf("CONST64(0x%02x%02x%02x%02x%02x%02x%02x%02x)",
              gf_mul(sbox(x), cir[y][0]),
              gf_mul(sbox(x), cir[y][1]),
              gf_mul(sbox(x), cir[y][2]),
              gf_mul(sbox(x), cir[y][3]),
              gf_mul(sbox(x), cir[y][4]),
              gf_mul(sbox(x), cir[y][5]),
              gf_mul(sbox(x), cir[y][6]),
              gf_mul(sbox(x), cir[y][7]));
           if (x < 255) printf(", ");
           if (!(++x & 3)) printf("\n");
       }
       printf("};\n\n");
  }
  
  printf("static const ulong64 cont[] = {\n");
  for (y = 0; y <= 10; y++) {
      printf("CONST64(0x");
      for (x = 0; x < 8; x++) {
         printf("%02x", sbox((8*y + x)&255));
      }
      printf("),\n");
  }
  printf("};\n\n");
  return 0;
   
}



/* $Source: /cvs/libtom/libtomcrypt/notes/etc/whirlgen.c,v $ */
/* $Revision: 1.2 $ */
/* $Date: 2005/05/05 14:35:58 $ */
