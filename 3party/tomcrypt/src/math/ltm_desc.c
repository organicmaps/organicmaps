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

#define DESC_DEF_ONLY
#include "tomcrypt.h"

#ifdef LTM_DESC

#include <tommath.h>

static const struct {
    int mpi_code, ltc_code;
} mpi_to_ltc_codes[] = {
   { MP_OKAY ,  CRYPT_OK},
   { MP_MEM  ,  CRYPT_MEM},
   { MP_VAL  ,  CRYPT_INVALID_ARG},
};

/**
   Convert a MPI error to a LTC error (Possibly the most powerful function ever!  Oh wait... no) 
   @param err    The error to convert
   @return The equivalent LTC error code or CRYPT_ERROR if none found
*/
static int mpi_to_ltc_error(int err)
{
   int x;

   for (x = 0; x < (int)(sizeof(mpi_to_ltc_codes)/sizeof(mpi_to_ltc_codes[0])); x++) {
       if (err == mpi_to_ltc_codes[x].mpi_code) { 
          return mpi_to_ltc_codes[x].ltc_code;
       }
   }
   return CRYPT_ERROR;
}

static int init(void **a)
{
   int err;

   LTC_ARGCHK(a != NULL);

   *a = XCALLOC(1, sizeof(mp_int));
   if (*a == NULL) {
      return CRYPT_MEM;
   }
   
   if ((err = mpi_to_ltc_error(mp_init(*a))) != CRYPT_OK) {
      XFREE(*a);
   }
   return err;
}

static void deinit(void *a)
{
   LTC_ARGCHKVD(a != NULL);
   mp_clear(a);
   XFREE(a);
}

static int neg(void *a, void *b)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_neg(a, b));
}

static int copy(void *a, void *b)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_copy(a, b));
}

static int init_copy(void **a, void *b)
{
   if (init(a) != CRYPT_OK) {
      return CRYPT_MEM;
   }
   return copy(b, *a);
}

/* ---- trivial ---- */
static int set_int(void *a, unsigned long b)
{
   LTC_ARGCHK(a != NULL);
   return mpi_to_ltc_error(mp_set_int(a, b));
}

static unsigned long get_int(void *a)
{
   LTC_ARGCHK(a != NULL);
   return mp_get_int(a);
}

static unsigned long get_digit(void *a, int n)
{
   mp_int *A;
   LTC_ARGCHK(a != NULL);
   A = a;
   return (n >= A->used || n < 0) ? 0 : A->dp[n];
}

static int get_digit_count(void *a)
{
   mp_int *A;
   LTC_ARGCHK(a != NULL);
   A = a;
   return A->used;
}
   
static int compare(void *a, void *b)
{
   int ret;
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   ret = mp_cmp(a, b);
   switch (ret) {
      case MP_LT: return LTC_MP_LT;
      case MP_EQ: return LTC_MP_EQ;
      case MP_GT: return LTC_MP_GT;
   }
   return 0;
}

static int compare_d(void *a, unsigned long b)
{
   int ret;
   LTC_ARGCHK(a != NULL);
   ret = mp_cmp_d(a, b);
   switch (ret) {
      case MP_LT: return LTC_MP_LT;
      case MP_EQ: return LTC_MP_EQ;
      case MP_GT: return LTC_MP_GT;
   }
   return 0;
}

static int count_bits(void *a)
{
   LTC_ARGCHK(a != NULL);
   return mp_count_bits(a);
}

static int count_lsb_bits(void *a)
{
   LTC_ARGCHK(a != NULL);
   return mp_cnt_lsb(a);
}


static int twoexpt(void *a, int n)
{
   LTC_ARGCHK(a != NULL);
   return mpi_to_ltc_error(mp_2expt(a, n));
}

/* ---- conversions ---- */

/* read ascii string */
static int read_radix(void *a, const char *b, int radix)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_read_radix(a, b, radix));
}

/* write one */
static int write_radix(void *a, char *b, int radix)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_toradix(a, b, radix));
}

/* get size as unsigned char string */
static unsigned long unsigned_size(void *a)
{
   LTC_ARGCHK(a != NULL);
   return mp_unsigned_bin_size(a);
}

/* store */
static int unsigned_write(void *a, unsigned char *b)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_to_unsigned_bin(a, b));
}

/* read */
static int unsigned_read(void *a, unsigned char *b, unsigned long len)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_read_unsigned_bin(a, b, len));
}

/* add */
static int add(void *a, void *b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_add(a, b, c));
}
  
static int addi(void *a, unsigned long b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_add_d(a, b, c));
}

/* sub */
static int sub(void *a, void *b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_sub(a, b, c));
}

static int subi(void *a, unsigned long b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_sub_d(a, b, c));
}

/* mul */
static int mul(void *a, void *b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_mul(a, b, c));
}

static int muli(void *a, unsigned long b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_mul_d(a, b, c));
}

/* sqr */
static int sqr(void *a, void *b)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_sqr(a, b));
}

/* div */
static int divide(void *a, void *b, void *c, void *d)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_div(a, b, c, d));
}

static int div_2(void *a, void *b)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_div_2(a, b));
}

/* modi */
static int modi(void *a, unsigned long b, unsigned long *c)
{
   mp_digit tmp;
   int      err;

   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(c != NULL);

   if ((err = mpi_to_ltc_error(mp_mod_d(a, b, &tmp))) != CRYPT_OK) {
      return err;
   }
   *c = tmp;
   return CRYPT_OK;
}  

/* gcd */
static int gcd(void *a, void *b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_gcd(a, b, c));
}

/* lcm */
static int lcm(void *a, void *b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_lcm(a, b, c));
}

static int mulmod(void *a, void *b, void *c, void *d)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   LTC_ARGCHK(d != NULL);
   return mpi_to_ltc_error(mp_mulmod(a,b,c,d));
}

static int sqrmod(void *a, void *b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_sqrmod(a,b,c));
}

/* invmod */
static int invmod(void *a, void *b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_invmod(a, b, c));
}

/* setup */
static int montgomery_setup(void *a, void **b)
{
   int err;
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   *b = XCALLOC(1, sizeof(mp_digit));
   if (*b == NULL) {
      return CRYPT_MEM;
   }
   if ((err = mpi_to_ltc_error(mp_montgomery_setup(a, (mp_digit *)*b))) != CRYPT_OK) {
      XFREE(*b);
   }
   return err;
}

/* get normalization value */
static int montgomery_normalization(void *a, void *b)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   return mpi_to_ltc_error(mp_montgomery_calc_normalization(a, b));
}

/* reduce */
static int montgomery_reduce(void *a, void *b, void *c)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   return mpi_to_ltc_error(mp_montgomery_reduce(a, b, *((mp_digit *)c)));
}

/* clean up */
static void montgomery_deinit(void *a)
{
   XFREE(a);
}

static int exptmod(void *a, void *b, void *c, void *d)
{
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   LTC_ARGCHK(c != NULL);
   LTC_ARGCHK(d != NULL);
   return mpi_to_ltc_error(mp_exptmod(a,b,c,d));
}   

static int isprime(void *a, int *b)
{
   int err;
   LTC_ARGCHK(a != NULL);
   LTC_ARGCHK(b != NULL);
   err = mpi_to_ltc_error(mp_prime_is_prime(a, 8, b));
   *b = (*b == MP_YES) ? LTC_MP_YES : LTC_MP_NO;
   return err;
}   

const ltc_math_descriptor ltm_desc = {

   "LibTomMath",
   (int)DIGIT_BIT,

   &init,
   &init_copy,
   &deinit,

   &neg,
   &copy,

   &set_int,
   &get_int,
   &get_digit,
   &get_digit_count,
   &compare,
   &compare_d,
   &count_bits,
   &count_lsb_bits,
   &twoexpt,

   &read_radix,
   &write_radix,
   &unsigned_size,
   &unsigned_write,
   &unsigned_read,

   &add,
   &addi,
   &sub,
   &subi,
   &mul,
   &muli,
   &sqr,
   &divide,
   &div_2,
   &modi,
   &gcd,
   &lcm,

   &mulmod,
   &sqrmod,
   &invmod,
   
   &montgomery_setup,
   &montgomery_normalization,
   &montgomery_reduce,
   &montgomery_deinit,

   &exptmod,
   &isprime,

#ifdef LTC_MECC
#ifdef LTC_MECC_FP
   &ltc_ecc_fp_mulmod,
#else   
   &ltc_ecc_mulmod,
#endif
   &ltc_ecc_projective_add_point,
   &ltc_ecc_projective_dbl_point,
   &ltc_ecc_map,
#ifdef LTC_ECC_SHAMIR
#ifdef LTC_MECC_FP
   &ltc_ecc_fp_mul2add,
#else
   &ltc_ecc_mul2add,
#endif /* LTC_MECC_FP */
#else
   NULL,
#endif /* LTC_ECC_SHAMIR */
#else
   NULL, NULL, NULL, NULL, NULL,
#endif /* LTC_MECC */

#ifdef LTC_MRSA
   &rsa_make_key,
   &rsa_exptmod,
#else
   NULL, NULL
#endif
};


#endif

/* $Source: /cvs/libtom/libtomcrypt/src/math/ltm_desc.c,v $ */
/* $Revision: 1.31 $ */
/* $Date: 2007/05/12 14:32:35 $ */
