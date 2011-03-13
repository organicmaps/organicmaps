/* emits an optimized version of LTC_SAFER+ ... only does encrypt so far... */

#include <stdio.h>
#include <string.h>

/* This is the "Armenian" Shuffle.  It takes the input from b and stores it in b2 */
#define SHUF\
    b2[0] = b[8]; b2[1] = b[11]; b2[2] = b[12]; b2[3] = b[15];   \
    b2[4] = b[2]; b2[5] = b[1]; b2[6] = b[6]; b2[7] = b[5];      \
    b2[8] = b[10]; b2[9] = b[9]; b2[10] = b[14]; b2[11] = b[13]; \
    b2[12] = b[0]; b2[13] = b[7]; b2[14] = b[4]; b2[15] = b[3]; memcpy(b, b2, sizeof(b));

/* This is the inverse shuffle.  It takes from b and gives to b2 */
#define iSHUF(b, b2)                                               \
    b2[0] = b[12]; b2[1] = b[5]; b2[2] = b[4]; b2[3] = b[15];      \
    b2[4] = b[14]; b2[5] = b[7]; b2[6] = b[6]; b2[7] = b[13];      \
    b2[8] = b[0]; b2[9] = b[9]; b2[10] = b[8]; b2[11] = b[1];      \
    b2[12] = b[2]; b2[13] = b[11]; b2[14] = b[10]; b2[15] = b[3]; memcpy(b, b2, sizeof(b));
    
#define ROUND(b, i)                                                                        \
    b[0]  = (safer_ebox[(b[0] ^ skey->saferp.K[i][0]) & 255] + skey->saferp.K[i+1][0]) & 255;    \
    b[1]  = safer_lbox[(b[1] + skey->saferp.K[i][1]) & 255] ^ skey->saferp.K[i+1][1];            \
    b[2]  = safer_lbox[(b[2] + skey->saferp.K[i][2]) & 255] ^ skey->saferp.K[i+1][2];            \
    b[3]  = (safer_ebox[(b[3] ^ skey->saferp.K[i][3]) & 255] + skey->saferp.K[i+1][3]) & 255;    \
    b[4]  = (safer_ebox[(b[4] ^ skey->saferp.K[i][4]) & 255] + skey->saferp.K[i+1][4]) & 255;    \
    b[5]  = safer_lbox[(b[5] + skey->saferp.K[i][5]) & 255] ^ skey->saferp.K[i+1][5];            \
    b[6]  = safer_lbox[(b[6] + skey->saferp.K[i][6]) & 255] ^ skey->saferp.K[i+1][6];            \
    b[7]  = (safer_ebox[(b[7] ^ skey->saferp.K[i][7]) & 255] + skey->saferp.K[i+1][7]) & 255;    \
    b[8]  = (safer_ebox[(b[8] ^ skey->saferp.K[i][8]) & 255] + skey->saferp.K[i+1][8]) & 255;    \
    b[9]  = safer_lbox[(b[9] + skey->saferp.K[i][9]) & 255] ^ skey->saferp.K[i+1][9];            \
    b[10] = safer_lbox[(b[10] + skey->saferp.K[i][10]) & 255] ^ skey->saferp.K[i+1][10];         \
    b[11] = (safer_ebox[(b[11] ^ skey->saferp.K[i][11]) & 255] + skey->saferp.K[i+1][11]) & 255; \
    b[12] = (safer_ebox[(b[12] ^ skey->saferp.K[i][12]) & 255] + skey->saferp.K[i+1][12]) & 255; \
    b[13] = safer_lbox[(b[13] + skey->saferp.K[i][13]) & 255] ^ skey->saferp.K[i+1][13];         \
    b[14] = safer_lbox[(b[14] + skey->saferp.K[i][14]) & 255] ^ skey->saferp.K[i+1][14];         \
    b[15] = (safer_ebox[(b[15] ^ skey->saferp.K[i][15]) & 255] + skey->saferp.K[i+1][15]) & 255;        

int main(void)
{
   int b[16], b2[16], x, y, z;
   
/* -- ENCRYPT ---  */
   for (x = 0; x < 16; x++) b[x] = x;
   /* emit encrypt preabmle  */
printf(
"void saferp_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)\n"
"{\n"
"   int x;\n"
"   unsigned char b[16];\n"
"\n"
"   LTC_ARGCHK(pt   != NULL);\n"
"   LTC_ARGCHK(ct   != NULL);\n"
"   LTC_ARGCHK(skey != NULL);\n"
"\n"
"   /* do eight rounds */\n"
"   for (x = 0; x < 16; x++) {\n"
"       b[x] = pt[x];\n"
"   }\n");   

   /* do 8 rounds of ROUND; LT; */
   for (x = 0; x < 8; x++) {
       /* ROUND(..., x*2) */
       for (y = 0; y < 16; y++) {
printf("b[%d] = (safer_%cbox[(b[%d] %c skey->saferp.K[%d][%d]) & 255] %c skey->saferp.K[%d][%d]) & 255;\n",
          b[y], "elle"[y&3], b[y], "^++^"[y&3],      x*2, y, "+^^+"[y&3], x*2+1, y);
       }
       
       /* LT */
       for (y = 0; y < 4; y++) {
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[0], b[0], b[1], b[0], b[1]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[2], b[2], b[3], b[3], b[2]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[4], b[4], b[5], b[5], b[4]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[6], b[6], b[7], b[7], b[6]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[8], b[8], b[9], b[9], b[8]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[10], b[10], b[11], b[11], b[10]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[12], b[12], b[13], b[13], b[12]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[14], b[14], b[15], b[15], b[14]);
      if (y < 3) {
         SHUF;
      }         
      }   
  }
  
printf(
"   if (skey->saferp.rounds <= 8) {\n");
/* finish */
   for (x = 0; x < 16; x++) {
   printf(
"      ct[%d] = (b[%d] %c skey->saferp.K[skey->saferp.rounds*2][%d]) & 255;\n",
       x, b[x], "^++^"[x&3], x);
   }   
   printf("      return;\n   }\n");
  
  /* 192-bit keys */
printf(  
"   /* 192-bit key? */\n"
"   if (skey->saferp.rounds > 8) {\n");
  
   /* do 4 rounds of ROUND; LT; */
   for (x = 8; x < 12; x++) {
       /* ROUND(..., x*2) */
       for (y = 0; y < 16; y++) {
printf("b[%d] = (safer_%cbox[(b[%d] %c skey->saferp.K[%d][%d]) & 255] %c skey->saferp.K[%d][%d]) & 255;\n",
          b[y], "elle"[y&3], b[y], "^++^"[y&3],      x*2, y, "+^^+"[y&3], x*2+1, y);
       }
       
       /* LT */
       for (y = 0; y < 4; y++) {
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[0], b[0], b[1], b[0], b[1]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[2], b[2], b[3], b[3], b[2]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[4], b[4], b[5], b[5], b[4]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[6], b[6], b[7], b[7], b[6]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[8], b[8], b[9], b[9], b[8]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[10], b[10], b[11], b[11], b[10]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[12], b[12], b[13], b[13], b[12]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[14], b[14], b[15], b[15], b[14]);
      if (y < 3) {
         SHUF;
      }         
      }   
  }
printf("}\n");
  
printf(
"   if (skey->saferp.rounds <= 12) {\n");
/* finish */
   for (x = 0; x < 16; x++) {
   printf(
"      ct[%d] = (b[%d] %c skey->saferp.K[skey->saferp.rounds*2][%d]) & 255;\n",
       x, b[x], "^++^"[x&3], x);
   }   
   printf("      return;\n   }\n");

  /* 256-bit keys */
printf(  
"   /* 256-bit key? */\n"
"   if (skey->saferp.rounds > 12) {\n");
  
   /* do 4 rounds of ROUND; LT; */
   for (x = 12; x < 16; x++) {
       /* ROUND(..., x*2) */
       for (y = 0; y < 16; y++) {
printf("b[%d] = (safer_%cbox[(b[%d] %c skey->saferp.K[%d][%d]) & 255] %c skey->saferp.K[%d][%d]) & 255;\n",
          b[y], "elle"[y&3], b[y], "^++^"[y&3],      x*2, y, "+^^+"[y&3], x*2+1, y);
       }
       
       /* LT */
       for (y = 0; y < 4; y++) {
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[0], b[0], b[1], b[0], b[1]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[2], b[2], b[3], b[3], b[2]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[4], b[4], b[5], b[5], b[4]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[6], b[6], b[7], b[7], b[6]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[8], b[8], b[9], b[9], b[8]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[10], b[10], b[11], b[11], b[10]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[12], b[12], b[13], b[13], b[12]);
printf("   b[%d]  = (b[%d] + (b[%d] = (b[%d] + b[%d]) & 255)) & 255;\n", b[14], b[14], b[15], b[15], b[14]);
      if (y < 3) {
         SHUF;
      }         
      }   
  }
/* finish */
   for (x = 0; x < 16; x++) {
   printf(
"      ct[%d] = (b[%d] %c skey->saferp.K[skey->saferp.rounds*2][%d]) & 255;\n",
       x, b[x], "^++^"[x&3], x);
   }   
   printf("   return;\n");
printf("   }\n}\n\n");

   return 0;
}


/* $Source: /cvs/libtom/libtomcrypt/notes/etc/saferp_optimizer.c,v $ */
/* $Revision: 1.3 $ */
/* $Date: 2007/05/12 14:20:27 $ */
