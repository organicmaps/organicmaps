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
#include "tomcrypt.h"

/**
  @file crypt.c
  Build strings, Tom St Denis
*/  

const char *crypt_build_settings =
   "LibTomCrypt " SCRYPT " (Tom St Denis, tomstdenis@gmail.com)\n"
   "LibTomCrypt is public domain software.\n"
   "Built on " __DATE__ " at " __TIME__ "\n\n\n"
   "Endianess: "
#if defined(ENDIAN_NEUTRAL)
   "neutral\n"
#elif defined(ENDIAN_LITTLE)
   "little"
   #if defined(ENDIAN_32BITWORD)
   " (32-bit words)\n"
   #else
   " (64-bit words)\n"
   #endif
#elif defined(ENDIAN_BIG)
   "big"
   #if defined(ENDIAN_32BITWORD)
   " (32-bit words)\n"
   #else
   " (64-bit words)\n"
   #endif
#endif
   "Clean stack: "
#if defined(LTC_CLEAN_STACK)
   "enabled\n"
#else
   "disabled\n"
#endif
   "Ciphers built-in:\n"
#if defined(LTC_BLOWFISH)
   "   Blowfish\n"
#endif
#if defined(LTC_RC2)
   "   LTC_RC2\n"
#endif
#if defined(LTC_RC5)
   "   LTC_RC5\n"
#endif
#if defined(LTC_RC6)
   "   LTC_RC6\n"
#endif
#if defined(LTC_SAFERP)
   "   Safer+\n"
#endif
#if defined(LTC_SAFER)
   "   Safer\n"
#endif
#if defined(LTC_RIJNDAEL)
   "   Rijndael\n"
#endif
#if defined(LTC_XTEA)
   "   LTC_XTEA\n"
#endif
#if defined(LTC_TWOFISH)
   "   Twofish "
   #if defined(LTC_TWOFISH_SMALL) && defined(LTC_TWOFISH_TABLES) && defined(LTC_TWOFISH_ALL_TABLES)
       "(small, tables, all_tables)\n"
   #elif defined(LTC_TWOFISH_SMALL) && defined(LTC_TWOFISH_TABLES)
       "(small, tables)\n"
   #elif defined(LTC_TWOFISH_SMALL) && defined(LTC_TWOFISH_ALL_TABLES)
       "(small, all_tables)\n"
   #elif defined(LTC_TWOFISH_TABLES) && defined(LTC_TWOFISH_ALL_TABLES)
       "(tables, all_tables)\n"
   #elif defined(LTC_TWOFISH_SMALL)
       "(small)\n"
   #elif defined(LTC_TWOFISH_TABLES)
       "(tables)\n"
   #elif defined(LTC_TWOFISH_ALL_TABLES)
       "(all_tables)\n"
   #else
       "\n"
   #endif
#endif
#if defined(LTC_DES)
   "   LTC_DES\n"
#endif
#if defined(LTC_CAST5)
   "   LTC_CAST5\n"
#endif
#if defined(LTC_NOEKEON)
   "   Noekeon\n"
#endif
#if defined(LTC_SKIPJACK)
   "   Skipjack\n"
#endif
#if defined(LTC_KHAZAD)
   "   Khazad\n"
#endif
#if defined(LTC_ANUBIS)
   "   Anubis "
#endif
#if defined(LTC_ANUBIS_TWEAK)
   " (tweaked)"
#endif
   "\n"
#if defined(LTC_KSEED)
   "   LTC_KSEED\n"
#endif
#if defined(LTC_KASUMI)
   "   KASUMI\n"
#endif

    "\nHashes built-in:\n"
#if defined(LTC_SHA512)
   "   LTC_SHA-512\n"
#endif
#if defined(LTC_SHA384)
   "   LTC_SHA-384\n"
#endif
#if defined(LTC_SHA256)
   "   LTC_SHA-256\n"
#endif
#if defined(LTC_SHA224)
   "   LTC_SHA-224\n"
#endif
#if defined(LTC_TIGER)
   "   LTC_TIGER\n"
#endif
#if defined(LTC_SHA1)
   "   LTC_SHA1\n"
#endif
#if defined(LTC_MD5)
   "   LTC_MD5\n"
#endif
#if defined(LTC_MD4)
   "   LTC_MD4\n"
#endif
#if defined(LTC_MD2)
   "   LTC_MD2\n"
#endif
#if defined(LTC_RIPEMD128)
   "   LTC_RIPEMD128\n"
#endif
#if defined(LTC_RIPEMD160)
   "   LTC_RIPEMD160\n"
#endif
#if defined(LTC_RIPEMD256)
   "   LTC_RIPEMD256\n"
#endif
#if defined(LTC_RIPEMD320)
   "   LTC_RIPEMD320\n"
#endif
#if defined(LTC_WHIRLPOOL)
   "   LTC_WHIRLPOOL\n"
#endif
#if defined(LTC_CHC_HASH)
   "   LTC_CHC_HASH \n"
#endif

    "\nBlock Chaining Modes:\n"
#if defined(LTC_CFB_MODE)
    "   CFB\n"
#endif
#if defined(LTC_OFB_MODE)
    "   OFB\n"
#endif
#if defined(LTC_ECB_MODE)
    "   ECB\n"
#endif
#if defined(LTC_CBC_MODE)
    "   CBC\n"
#endif
#if defined(LTC_CTR_MODE)
    "   CTR "
#endif
#if defined(LTC_CTR_OLD)
    " (CTR_OLD) "
#endif
    "\n"
#if defined(LRW_MODE)
    "   LRW_MODE"
#if defined(LRW_TABLES)
    " (LRW_TABLES) "
#endif
    "\n"
#endif
#if defined(LTC_F8_MODE)
    "   F8 MODE\n"
#endif    
#if defined(LTC_XTS_MODE)
    "   LTC_XTS_MODE\n"
#endif

    "\nMACs:\n"
#if defined(LTC_HMAC)
    "   LTC_HMAC\n"
#endif
#if defined(LTC_OMAC)
    "   LTC_OMAC\n"
#endif
#if defined(LTC_PMAC)
    "   PMAC\n"
#endif
#if defined(LTC_PELICAN)
    "   LTC_PELICAN\n"
#endif
#if defined(LTC_XCBC)
    "   XCBC-MAC\n"
#endif
#if defined(LTC_F9_MODE)
    "   F9-MAC\n"
#endif

    "\nENC + AUTH modes:\n"
#if defined(LTC_EAX_MODE)
    "   LTC_EAX_MODE\n"
#endif
#if defined(LTC_OCB_MODE)
    "   LTC_OCB_MODE\n"
#endif
#if defined(LTC_CCM_MODE)
    "   LTC_CCM_MODE\n"
#endif
#if defined(LTC_GCM_MODE)
    "   LTC_GCM_MODE "
#endif
#if defined(LTC_GCM_TABLES)
    " (LTC_GCM_TABLES) "
#endif
   "\n"

    "\nPRNG:\n"
#if defined(LTC_YARROW)
    "   Yarrow\n"
#endif
#if defined(LTC_SPRNG)
    "   LTC_SPRNG\n"
#endif
#if defined(LTC_RC4)
    "   LTC_RC4\n"
#endif
#if defined(LTC_FORTUNA)
    "   Fortuna\n"
#endif
#if defined(LTC_SOBER128)
    "   LTC_SOBER128\n"
#endif

    "\nPK Algs:\n"
#if defined(LTC_MRSA)
    "   RSA \n"
#endif
#if defined(LTC_MECC)
    "   ECC\n"
#endif
#if defined(LTC_MDSA)
    "   DSA\n"
#endif
#if defined(MKAT)
    "   Katja\n"
#endif    

    "\nCompiler:\n"
#if defined(WIN32)
    "   WIN32 platform detected.\n"
#endif
#if defined(__CYGWIN__)
    "   CYGWIN Detected.\n"
#endif
#if defined(__DJGPP__)
    "   DJGPP Detected.\n"
#endif
#if defined(_MSC_VER)
    "   MSVC compiler detected.\n"
#endif
#if defined(__GNUC__)
    "   GCC compiler detected.\n"
#endif
#if defined(INTEL_CC)
    "   Intel C Compiler detected.\n"
#endif
#if defined(__x86_64__)
    "   x86-64 detected.\n"
#endif
#if defined(LTC_PPC32)
    "   LTC_PPC32 defined \n"
#endif    

    "\nVarious others: "
#if defined(LTC_BASE64)
    " LTC_BASE64 "
#endif
#if defined(MPI)
    " MPI "
#endif
#if defined(TRY_UNRANDOM_FIRST)
    " TRY_UNRANDOM_FIRST "
#endif
#if defined(LTC_TEST)
    " LTC_TEST "
#endif
#if defined(LTC_PKCS_1)
    " LTC_PKCS#1 "
#endif
#if defined(LTC_PKCS_5)
    " LTC_PKCS#5 "
#endif
#if defined(LTC_SMALL_CODE)
    " LTC_SMALL_CODE "
#endif
#if defined(LTC_NO_FILE)
    " LTC_NO_FILE "
#endif
#if defined(LTC_DER)
    " LTC_DER "
#endif
#if defined(LTC_FAST)
    " LTC_FAST "
#endif
#if defined(LTC_NO_FAST)
    " LTC_NO_FAST "
#endif
#if defined(LTC_NO_BSWAP)
    " LTC_NO_BSWAP "
#endif
#if defined(LTC_NO_ASM)
    " LTC_NO_ASM "
#endif
#if defined(LTC_NO_TEST)
    " LTC_NO_TEST "
#endif
#if defined(LTC_NO_TABLES)
    " LTC_NO_TABLES "
#endif
#if defined(LTC_PTHREAD)
    " LTC_PTHREAD "
#endif
#if defined(LTM_LTC_DESC)
    " LTM_DESC "
#endif
#if defined(TFM_LTC_DESC)
    " TFM_DESC "
#endif
#if defined(LTC_MECC_ACCEL)
    " LTC_MECC_ACCEL "
#endif
#if defined(GMP_LTC_DESC)
    " GMP_DESC "
#endif
#if defined(LTC_EASY)
    " (easy) "
#endif    
#if defined(LTC_MECC_FP)
   " LTC_MECC_FP "
#endif
#if defined(LTC_ECC_SHAMIR)
   " LTC_ECC_SHAMIR "
#endif
    "\n"
    "\n\n\n"
    ;


/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt.c,v $ */
/* $Revision: 1.36 $ */
/* $Date: 2007/05/12 14:46:12 $ */
