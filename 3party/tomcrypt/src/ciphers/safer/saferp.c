/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/**
   @file saferp.c
   LTC_SAFER+ Implementation by Tom St Denis
*/
#include "tomcrypt.h"

#ifdef LTC_SAFERP

#define __LTC_SAFER_TAB_C__
#include "safer_tab.c"

const struct ltc_cipher_descriptor saferp_desc =
{
    "safer+",
    4,
    16, 32, 16, 8,
    &saferp_setup,
    &saferp_ecb_encrypt,
    &saferp_ecb_decrypt,
    &saferp_test,
    &saferp_done,
    &saferp_keysize,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/* ROUND(b,i)
 *
 * This is one forward key application.  Note the basic form is
 * key addition, substitution, key addition.  The safer_ebox and safer_lbox
 * are the exponentiation box and logarithm boxes respectively.
 * The value of 'i' is the current round number which allows this
 * function to be unrolled massively.  Most of LTC_SAFER+'s speed
 * comes from not having to compute indirect accesses into the
 * array of 16 bytes b[0..15] which is the block of data
*/

#define ROUND(b, i) do {                                                                         \
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
    b[15] = (safer_ebox[(b[15] ^ skey->saferp.K[i][15]) & 255] + skey->saferp.K[i+1][15]) & 255; \
} while (0)

/* This is one inverse key application */
#define iROUND(b, i) do {                                                                        \
    b[0]  = safer_lbox[(b[0] - skey->saferp.K[i+1][0]) & 255] ^ skey->saferp.K[i][0];            \
    b[1]  = (safer_ebox[(b[1] ^ skey->saferp.K[i+1][1]) & 255] - skey->saferp.K[i][1]) & 255;    \
    b[2]  = (safer_ebox[(b[2] ^ skey->saferp.K[i+1][2]) & 255] - skey->saferp.K[i][2]) & 255;    \
    b[3]  = safer_lbox[(b[3] - skey->saferp.K[i+1][3]) & 255] ^ skey->saferp.K[i][3];            \
    b[4]  = safer_lbox[(b[4] - skey->saferp.K[i+1][4]) & 255] ^ skey->saferp.K[i][4];            \
    b[5]  = (safer_ebox[(b[5] ^ skey->saferp.K[i+1][5]) & 255] - skey->saferp.K[i][5]) & 255;    \
    b[6]  = (safer_ebox[(b[6] ^ skey->saferp.K[i+1][6]) & 255] - skey->saferp.K[i][6]) & 255;    \
    b[7]  = safer_lbox[(b[7] - skey->saferp.K[i+1][7]) & 255] ^ skey->saferp.K[i][7];            \
    b[8]  = safer_lbox[(b[8] - skey->saferp.K[i+1][8]) & 255] ^ skey->saferp.K[i][8];            \
    b[9]  = (safer_ebox[(b[9] ^ skey->saferp.K[i+1][9]) & 255] - skey->saferp.K[i][9]) & 255;    \
    b[10] = (safer_ebox[(b[10] ^ skey->saferp.K[i+1][10]) & 255] - skey->saferp.K[i][10]) & 255; \
    b[11] = safer_lbox[(b[11] - skey->saferp.K[i+1][11]) & 255] ^ skey->saferp.K[i][11];         \
    b[12] = safer_lbox[(b[12] - skey->saferp.K[i+1][12]) & 255] ^ skey->saferp.K[i][12];         \
    b[13] = (safer_ebox[(b[13] ^ skey->saferp.K[i+1][13]) & 255] - skey->saferp.K[i][13]) & 255; \
    b[14] = (safer_ebox[(b[14] ^ skey->saferp.K[i+1][14]) & 255] - skey->saferp.K[i][14]) & 255; \
    b[15] = safer_lbox[(b[15] - skey->saferp.K[i+1][15]) & 255] ^ skey->saferp.K[i][15];         \
} while (0)

/* This is a forward single layer PHT transform.  */
#define PHT(b) do {                                          \
    b[0]  = (b[0] + (b[1] = (b[0] + b[1]) & 255)) & 255;     \
    b[2]  = (b[2] + (b[3] = (b[3] + b[2]) & 255)) & 255;     \
    b[4]  = (b[4] + (b[5] = (b[5] + b[4]) & 255)) & 255;     \
    b[6]  = (b[6] + (b[7] = (b[7] + b[6]) & 255)) & 255;     \
    b[8]  = (b[8] + (b[9] = (b[9] + b[8]) & 255)) & 255;     \
    b[10] = (b[10] + (b[11] = (b[11] + b[10]) & 255)) & 255; \
    b[12] = (b[12] + (b[13] = (b[13] + b[12]) & 255)) & 255; \
    b[14] = (b[14] + (b[15] = (b[15] + b[14]) & 255)) & 255; \
} while (0)

/* This is an inverse single layer PHT transform */
#define iPHT(b) do {                                          \
    b[15] = (b[15] - (b[14] = (b[14] - b[15]) & 255)) & 255;  \
    b[13] = (b[13] - (b[12] = (b[12] - b[13]) & 255)) & 255;  \
    b[11] = (b[11] - (b[10] = (b[10] - b[11]) & 255)) & 255;  \
    b[9]  = (b[9] - (b[8] = (b[8] - b[9]) & 255)) & 255;      \
    b[7]  = (b[7] - (b[6] = (b[6] - b[7]) & 255)) & 255;      \
    b[5]  = (b[5] - (b[4] = (b[4] - b[5]) & 255)) & 255;      \
    b[3]  = (b[3] - (b[2] = (b[2] - b[3]) & 255)) & 255;      \
    b[1]  = (b[1] - (b[0] = (b[0] - b[1]) & 255)) & 255;      \
 } while (0)

/* This is the "Armenian" Shuffle.  It takes the input from b and stores it in b2 */
#define SHUF(b, b2) do {                                         \
    b2[0] = b[8]; b2[1] = b[11]; b2[2] = b[12]; b2[3] = b[15];   \
    b2[4] = b[2]; b2[5] = b[1]; b2[6] = b[6]; b2[7] = b[5];      \
    b2[8] = b[10]; b2[9] = b[9]; b2[10] = b[14]; b2[11] = b[13]; \
    b2[12] = b[0]; b2[13] = b[7]; b2[14] = b[4]; b2[15] = b[3];  \
} while (0)

/* This is the inverse shuffle.  It takes from b and gives to b2 */
#define iSHUF(b, b2) do {                                          \
    b2[0] = b[12]; b2[1] = b[5]; b2[2] = b[4]; b2[3] = b[15];      \
    b2[4] = b[14]; b2[5] = b[7]; b2[6] = b[6]; b2[7] = b[13];      \
    b2[8] = b[0]; b2[9] = b[9]; b2[10] = b[8]; b2[11] = b[1];      \
    b2[12] = b[2]; b2[13] = b[11]; b2[14] = b[10]; b2[15] = b[3];  \
} while (0)

/* The complete forward Linear Transform layer.
 * Note that alternating usage of b and b2.
 * Each round of LT starts in 'b' and ends in 'b2'.
 */
#define LT(b, b2) do {        \
    PHT(b);  SHUF(b, b2);     \
    PHT(b2); SHUF(b2, b);     \
    PHT(b);  SHUF(b, b2);     \
    PHT(b2);                  \
} while (0)

/* This is the inverse linear transform layer.  */
#define iLT(b, b2) do {       \
    iPHT(b);                  \
    iSHUF(b, b2); iPHT(b2);   \
    iSHUF(b2, b); iPHT(b);    \
    iSHUF(b, b2); iPHT(b2);   \
} while (0)

#ifdef LTC_SMALL_CODE

static void _round(unsigned char *b, int i, symmetric_key *skey)
{
   ROUND(b, i);
}

static void _iround(unsigned char *b, int i, symmetric_key *skey)
{
   iROUND(b, i);
}

static void _lt(unsigned char *b, unsigned char *b2)
{
   LT(b, b2);
}

static void _ilt(unsigned char *b, unsigned char *b2)
{
   iLT(b, b2);
}

#undef ROUND
#define ROUND(b, i) _round(b, i, skey)

#undef iROUND
#define iROUND(b, i) _iround(b, i, skey)

#undef LT
#define LT(b, b2) _lt(b, b2)

#undef iLT
#define iLT(b, b2) _ilt(b, b2)

#endif

/* These are the 33, 128-bit bias words for the key schedule */
static const unsigned char safer_bias[33][16] = {
{  70, 151, 177, 186, 163, 183,  16,  10, 197,  55, 179, 201,  90,  40, 172, 100},
{ 236, 171, 170, 198, 103, 149,  88,  13, 248, 154, 246, 110, 102, 220,   5,  61},
{ 138, 195, 216, 137, 106, 233,  54,  73,  67, 191, 235, 212, 150, 155, 104, 160},
{  93,  87, 146,  31, 213, 113,  92, 187,  34, 193, 190, 123, 188, 153,  99, 148},
{  42,  97, 184,  52,  50,  25, 253, 251,  23,  64, 230,  81,  29,  65,  68, 143},
{ 221,   4, 128, 222, 231,  49, 214, 127,   1, 162, 247,  57, 218, 111,  35, 202},
{  58, 208,  28, 209,  48,  62,  18, 161, 205,  15, 224, 168, 175, 130,  89,  44},
{ 125, 173, 178, 239, 194, 135, 206, 117,   6,  19,   2, 144,  79,  46, 114,  51},
{ 192, 141, 207, 169, 129, 226, 196,  39,  47, 108, 122, 159,  82, 225,  21,  56},
{ 252,  32,  66, 199,   8, 228,   9,  85,  94, 140,  20, 118,  96, 255, 223, 215},
{ 250,  11,  33,   0,  26, 249, 166, 185, 232, 158,  98,  76, 217, 145,  80, 210},
{  24, 180,   7, 132, 234,  91, 164, 200,  14, 203,  72, 105,  75,  78, 156,  53},
{  69,  77,  84, 229,  37,  60,  12,  74, 139,  63, 204, 167, 219, 107, 174, 244},
{  45, 243, 124, 109, 157, 181,  38, 116, 242, 147,  83, 176, 240,  17, 237, 131},
{ 182,   3,  22, 115,  59,  30, 142, 112, 189, 134,  27,  71, 126,  36,  86, 241},
{ 136,  70, 151, 177, 186, 163, 183,  16,  10, 197,  55, 179, 201,  90,  40, 172},
{ 220, 134, 119, 215, 166,  17, 251, 244, 186, 146, 145, 100, 131, 241,  51, 239},
{  44, 181, 178,  43, 136, 209, 153, 203, 140, 132,  29,  20, 129, 151, 113, 202},
{ 163, 139,  87,  60, 130, 196,  82,  92,  28, 232, 160,   4, 180, 133,  74, 246},
{  84, 182, 223,  12,  26, 142, 222, 224,  57, 252,  32, 155,  36,  78, 169, 152},
{ 171, 242,  96, 208, 108, 234, 250, 199, 217,   0, 212,  31, 110,  67, 188, 236},
{ 137, 254, 122,  93,  73, 201,  50, 194, 249, 154, 248, 109,  22, 219,  89, 150},
{ 233, 205, 230,  70,  66, 143,  10, 193, 204, 185, 101, 176, 210, 198, 172,  30},
{  98,  41,  46,  14, 116,  80,   2,  90, 195,  37, 123, 138,  42,  91, 240,   6},
{  71, 111, 112, 157, 126,  16, 206,  18,  39, 213,  76,  79, 214, 121,  48, 104},
{ 117, 125, 228, 237, 128, 106, 144,  55, 162,  94, 118, 170, 197, 127,  61, 175},
{ 229,  25,  97, 253,  77, 124, 183,  11, 238, 173,  75,  34, 245, 231, 115,  35},
{ 200,   5, 225, 102, 221, 179,  88, 105,  99,  86,  15, 161,  49, 149,  23,   7},
{  40,   1,  45, 226, 147, 190,  69,  21, 174, 120,   3, 135, 164, 184,  56, 207},
{   8, 103,   9, 148, 235,  38, 168, 107, 189,  24,  52,  27, 187, 191, 114, 247},
{  53,  72, 156,  81,  47,  59,  85, 227, 192, 159, 216, 211, 243, 141, 177, 255},
{  62, 220, 134, 119, 215, 166,  17, 251, 244, 186, 146, 145, 100, 131, 241,  51}};

 /**
    Initialize the LTC_SAFER+ block cipher
    @param key The symmetric key you wish to pass
    @param keylen The key length in bytes
    @param num_rounds The number of rounds desired (0 for default)
    @param skey The key in as scheduled by this function.
    @return CRYPT_OK if successful
 */
int saferp_setup(const unsigned char *key, int keylen, int num_rounds, symmetric_key *skey)
{
   unsigned x, y, z;
   unsigned char t[33];
   static const int rounds[3] = { 8, 12, 16 };

   LTC_ARGCHK(key  != NULL);
   LTC_ARGCHK(skey != NULL);

   /* check arguments */
   if (keylen != 16 && keylen != 24 && keylen != 32) {
      return CRYPT_INVALID_KEYSIZE;
   }

   /* Is the number of rounds valid?  Either use zero for default or
    * 8,12,16 rounds for 16,24,32 byte keys
    */
   if (num_rounds != 0 && num_rounds != rounds[(keylen/8)-2]) {
      return CRYPT_INVALID_ROUNDS;
   }

   /* 128 bit key version */
   if (keylen == 16) {
       /* copy key into t */
       for (x = y = 0; x < 16; x++) {
           t[x] = key[x];
           y ^= key[x];
       }
       t[16] = y;

       /* make round keys */
       for (x = 0; x < 16; x++) {
           skey->saferp.K[0][x] = t[x];
       }

       /* make the 16 other keys as a transformation of the first key */
       for (x = 1; x < 17; x++) {
           /* rotate 3 bits each */
           for (y = 0; y < 17; y++) {
               t[y] = ((t[y]<<3)|(t[y]>>5)) & 255;
           }

           /* select and add */
           z = x;
           for (y = 0; y < 16; y++) {
               skey->saferp.K[x][y] = (t[z] + safer_bias[x-1][y]) & 255;
               if (++z == 17) { z = 0; }
           }
       }
       skey->saferp.rounds = 8;
   } else if (keylen == 24) {
       /* copy key into t */
       for (x = y = 0; x < 24; x++) {
           t[x] = key[x];
           y ^= key[x];
       }
       t[24] = y;

       /* make round keys */
       for (x = 0; x < 16; x++) {
           skey->saferp.K[0][x] = t[x];
       }

       for (x = 1; x < 25; x++) {
           /* rotate 3 bits each */
           for (y = 0; y < 25; y++) {
               t[y] = ((t[y]<<3)|(t[y]>>5)) & 255;
           }

           /* select and add */
           z = x;
           for (y = 0; y < 16; y++) {
               skey->saferp.K[x][y] = (t[z] + safer_bias[x-1][y]) & 255;
               if (++z == 25) { z = 0; }
           }
       }
       skey->saferp.rounds = 12;
   } else {
       /* copy key into t */
       for (x = y = 0; x < 32; x++) {
           t[x] = key[x];
           y ^= key[x];
       }
       t[32] = y;

       /* make round keys */
       for (x = 0; x < 16; x++) {
           skey->saferp.K[0][x] = t[x];
       }

       for (x = 1; x < 33; x++) {
           /* rotate 3 bits each */
           for (y = 0; y < 33; y++) {
               t[y] = ((t[y]<<3)|(t[y]>>5)) & 255;
           }

           /* select and add */
           z = x;
           for (y = 0; y < 16; y++) {
               skey->saferp.K[x][y] = (t[z] + safer_bias[x-1][y]) & 255;
               if (++z == 33) { z = 0; }
           }
       }
       skey->saferp.rounds = 16;
   }
#ifdef LTC_CLEAN_STACK
   zeromem(t, sizeof(t));
#endif
   return CRYPT_OK;
}

/**
  Encrypts a block of text with LTC_SAFER+
  @param pt The input plaintext (16 bytes)
  @param ct The output ciphertext (16 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
int saferp_ecb_encrypt(const unsigned char *pt, unsigned char *ct, symmetric_key *skey)
{
   unsigned char b[16];
   int x;

   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);

   /* do eight rounds */
   for (x = 0; x < 16; x++) {
       b[x] = pt[x];
   }
   ROUND(b,  0);  LT(b, ct);
   ROUND(ct, 2);  LT(ct, b);
   ROUND(b,  4);  LT(b, ct);
   ROUND(ct, 6);  LT(ct, b);
   ROUND(b,  8);  LT(b, ct);
   ROUND(ct, 10); LT(ct, b);
   ROUND(b,  12); LT(b, ct);
   ROUND(ct, 14); LT(ct, b);
   /* 192-bit key? */
   if (skey->saferp.rounds > 8) {
      ROUND(b, 16);  LT(b, ct);
      ROUND(ct, 18); LT(ct, b);
      ROUND(b, 20);  LT(b, ct);
      ROUND(ct, 22); LT(ct, b);
   }
   /* 256-bit key? */
   if (skey->saferp.rounds > 12) {
      ROUND(b, 24);  LT(b, ct);
      ROUND(ct, 26); LT(ct, b);
      ROUND(b, 28);  LT(b, ct);
      ROUND(ct, 30); LT(ct, b);
   }
   ct[0] = b[0] ^ skey->saferp.K[skey->saferp.rounds*2][0];
   ct[1] = (b[1] + skey->saferp.K[skey->saferp.rounds*2][1]) & 255;
   ct[2] = (b[2] + skey->saferp.K[skey->saferp.rounds*2][2]) & 255;
   ct[3] = b[3] ^ skey->saferp.K[skey->saferp.rounds*2][3];
   ct[4] = b[4] ^ skey->saferp.K[skey->saferp.rounds*2][4];
   ct[5] = (b[5] + skey->saferp.K[skey->saferp.rounds*2][5]) & 255;
   ct[6] = (b[6] + skey->saferp.K[skey->saferp.rounds*2][6]) & 255;
   ct[7] = b[7] ^ skey->saferp.K[skey->saferp.rounds*2][7];
   ct[8] = b[8] ^ skey->saferp.K[skey->saferp.rounds*2][8];
   ct[9] = (b[9] + skey->saferp.K[skey->saferp.rounds*2][9]) & 255;
   ct[10] = (b[10] + skey->saferp.K[skey->saferp.rounds*2][10]) & 255;
   ct[11] = b[11] ^ skey->saferp.K[skey->saferp.rounds*2][11];
   ct[12] = b[12] ^ skey->saferp.K[skey->saferp.rounds*2][12];
   ct[13] = (b[13] + skey->saferp.K[skey->saferp.rounds*2][13]) & 255;
   ct[14] = (b[14] + skey->saferp.K[skey->saferp.rounds*2][14]) & 255;
   ct[15] = b[15] ^ skey->saferp.K[skey->saferp.rounds*2][15];
#ifdef LTC_CLEAN_STACK
   zeromem(b, sizeof(b));
#endif
   return CRYPT_OK;
}

/**
  Decrypts a block of text with LTC_SAFER+
  @param ct The input ciphertext (16 bytes)
  @param pt The output plaintext (16 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/
int saferp_ecb_decrypt(const unsigned char *ct, unsigned char *pt, symmetric_key *skey)
{
   unsigned char b[16];
   int x;

   LTC_ARGCHK(pt   != NULL);
   LTC_ARGCHK(ct   != NULL);
   LTC_ARGCHK(skey != NULL);

   /* do eight rounds */
   b[0] = ct[0] ^ skey->saferp.K[skey->saferp.rounds*2][0];
   b[1] = (ct[1] - skey->saferp.K[skey->saferp.rounds*2][1]) & 255;
   b[2] = (ct[2] - skey->saferp.K[skey->saferp.rounds*2][2]) & 255;
   b[3] = ct[3] ^ skey->saferp.K[skey->saferp.rounds*2][3];
   b[4] = ct[4] ^ skey->saferp.K[skey->saferp.rounds*2][4];
   b[5] = (ct[5] - skey->saferp.K[skey->saferp.rounds*2][5]) & 255;
   b[6] = (ct[6] - skey->saferp.K[skey->saferp.rounds*2][6]) & 255;
   b[7] = ct[7] ^ skey->saferp.K[skey->saferp.rounds*2][7];
   b[8] = ct[8] ^ skey->saferp.K[skey->saferp.rounds*2][8];
   b[9] = (ct[9] - skey->saferp.K[skey->saferp.rounds*2][9]) & 255;
   b[10] = (ct[10] - skey->saferp.K[skey->saferp.rounds*2][10]) & 255;
   b[11] = ct[11] ^ skey->saferp.K[skey->saferp.rounds*2][11];
   b[12] = ct[12] ^ skey->saferp.K[skey->saferp.rounds*2][12];
   b[13] = (ct[13] - skey->saferp.K[skey->saferp.rounds*2][13]) & 255;
   b[14] = (ct[14] - skey->saferp.K[skey->saferp.rounds*2][14]) & 255;
   b[15] = ct[15] ^ skey->saferp.K[skey->saferp.rounds*2][15];
   /* 256-bit key? */
   if (skey->saferp.rounds > 12) {
      iLT(b, pt); iROUND(pt, 30);
      iLT(pt, b); iROUND(b, 28);
      iLT(b, pt); iROUND(pt, 26);
      iLT(pt, b); iROUND(b, 24);
   }
   /* 192-bit key? */
   if (skey->saferp.rounds > 8) {
      iLT(b, pt); iROUND(pt, 22);
      iLT(pt, b); iROUND(b, 20);
      iLT(b, pt); iROUND(pt, 18);
      iLT(pt, b); iROUND(b, 16);
   }
   iLT(b, pt); iROUND(pt, 14);
   iLT(pt, b); iROUND(b, 12);
   iLT(b, pt); iROUND(pt,10);
   iLT(pt, b); iROUND(b, 8);
   iLT(b, pt); iROUND(pt,6);
   iLT(pt, b); iROUND(b, 4);
   iLT(b, pt); iROUND(pt,2);
   iLT(pt, b); iROUND(b, 0);
   for (x = 0; x < 16; x++) {
       pt[x] = b[x];
   }
#ifdef LTC_CLEAN_STACK
   zeromem(b, sizeof(b));
#endif
   return CRYPT_OK;
}

/**
  Performs a self-test of the LTC_SAFER+ block cipher
  @return CRYPT_OK if functional, CRYPT_NOP if self-test has been disabled
*/
int saferp_test(void)
{
 #ifndef LTC_TEST
    return CRYPT_NOP;
 #else
   static const struct {
       int keylen;
       unsigned char key[32], pt[16], ct[16];
   } tests[] = {
       {
           16,
           { 41, 35, 190, 132, 225, 108, 214, 174,
             82, 144, 73, 241, 241, 187, 233, 235 },
           { 179, 166, 219, 60, 135, 12, 62, 153,
             36, 94, 13, 28, 6, 183, 71, 222 },
           { 224, 31, 182, 10, 12, 255, 84, 70,
             127, 13, 89, 249, 9, 57, 165, 220 }
       }, {
           24,
           { 72, 211, 143, 117, 230, 217, 29, 42,
             229, 192, 247, 43, 120, 129, 135, 68,
             14, 95, 80, 0, 212, 97, 141, 190 },
           { 123, 5, 21, 7, 59, 51, 130, 31,
             24, 112, 146, 218, 100, 84, 206, 177 },
           { 92, 136, 4, 63, 57, 95, 100, 0,
             150, 130, 130, 16, 193, 111, 219, 133 }
       }, {
           32,
           { 243, 168, 141, 254, 190, 242, 235, 113,
             255, 160, 208, 59, 117, 6, 140, 126,
             135, 120, 115, 77, 208, 190, 130, 190,
             219, 194, 70, 65, 43, 140, 250, 48 },
           { 127, 112, 240, 167, 84, 134, 50, 149,
             170, 91, 104, 19, 11, 230, 252, 245 },
           { 88, 11, 25, 36, 172, 229, 202, 213,
             170, 65, 105, 153, 220, 104, 153, 138 }
       }
    };

   unsigned char tmp[2][16];
   symmetric_key skey;
   int err, i, y;

   for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
      if ((err = saferp_setup(tests[i].key, tests[i].keylen, 0, &skey)) != CRYPT_OK)  {
         return err;
      }
      saferp_ecb_encrypt(tests[i].pt, tmp[0], &skey);
      saferp_ecb_decrypt(tmp[0], tmp[1], &skey);

      /* compare */
      if (XMEMCMP(tmp[0], tests[i].ct, 16) || XMEMCMP(tmp[1], tests[i].pt, 16)) {
         return CRYPT_FAIL_TESTVECTOR;
      }

      /* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
      for (y = 0; y < 16; y++) tmp[0][y] = 0;
      for (y = 0; y < 1000; y++) saferp_ecb_encrypt(tmp[0], tmp[0], &skey);
      for (y = 0; y < 1000; y++) saferp_ecb_decrypt(tmp[0], tmp[0], &skey);
      for (y = 0; y < 16; y++) if (tmp[0][y] != 0) return CRYPT_FAIL_TESTVECTOR;
   }

   return CRYPT_OK;
 #endif
}

/** Terminate the context
   @param skey    The scheduled key
*/
void saferp_done(symmetric_key *skey)
{
  LTC_UNUSED_PARAM(skey);
}

/**
  Gets suitable key size
  @param keysize [in/out] The length of the recommended key (in bytes).  This function will store the suitable size back in this variable.
  @return CRYPT_OK if the input key size is acceptable.
*/
int saferp_keysize(int *keysize)
{
   LTC_ARGCHK(keysize != NULL);

   if (*keysize < 16)
      return CRYPT_INVALID_KEYSIZE;
   if (*keysize < 24) {
      *keysize = 16;
   } else if (*keysize < 32) {
      *keysize = 24;
   } else {
      *keysize = 32;
   }
   return CRYPT_OK;
}

#endif



/* $Source$ */
/* $Revision$ */
/* $Date$ */
