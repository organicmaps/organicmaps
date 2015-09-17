
#ifndef __TEST_H_
#define __TEST_H_

#include <tomcrypt.h>

#ifdef USE_LTM
/* Use libtommath as MPI provider */
#elif defined(USE_TFM)
/* Use tomsfastmath as MPI provider */
#elif defined(USE_GMP)
/* Use GNU Multiple Precision Arithmetic Library as MPI provider */
#else
/* The user must define his own MPI provider! */
#ifndef EXT_MATH_LIB
/*
 * Yes, you're right, you could also name your instance of the MPI provider
 * "EXT_MATH_LIB" and you wouldn't need to define it, but most users won't do
 * this and so it's treated as an error and you have to comment out the
 * following statement :)
 */
#error EXT_MATH_LIB is required to be defined
#endif
#endif

/* enable stack testing */
/* #define STACK_TEST */

/* stack testing, define this if stack usage goes downwards [e.g. x86] */
#define STACK_DOWN

typedef struct {
    char *name, *prov, *req;
    int  (*entry)(void);
} test_entry;

extern prng_state yarrow_prng;

void run_cmd(int res, int line, char *file, char *cmd, const char *algorithm);

#ifdef LTC_VERBOSE
#define DO(x) do { fprintf(stderr, "%s:\n", #x); run_cmd((x), __LINE__, __FILE__, #x, NULL); } while (0)
#define DOX(x, str) do { fprintf(stderr, "%s - %s:\n", #x, (str)); run_cmd((x), __LINE__, __FILE__, #x, (str)); } while (0)
#else
#define DO(x) do { run_cmd((x), __LINE__, __FILE__, #x, NULL); } while (0)
#define DOX(x, str) do { run_cmd((x), __LINE__, __FILE__, #x, (str)); } while (0)
#endif

/* TESTS */
int cipher_hash_test(void);
int modes_test(void);
int mac_test(void);
int pkcs_1_test(void);
int pkcs_1_pss_test(void);
int pkcs_1_oaep_test(void);
int pkcs_1_emsa_test(void);
int pkcs_1_eme_test(void);
int store_test(void);
int rsa_test(void);
int dh_test(void);
int katja_test(void);
int ecc_tests(void);
int dsa_test(void);
int der_tests(void);
int misc_test(void);
int base64_test(void);

/* timing */
#define KTIMES  25
#define TIMES   100000

extern struct list {
    int id;
    unsigned long spd1, spd2, avg;
} results[];

extern int no_results;

#ifdef LTC_PKCS_1
extern const struct ltc_prng_descriptor no_prng_desc;
#endif

void print_hex(const char* what, const unsigned char* p, const unsigned long l);
int sorter(const void *a, const void *b);
void tally_results(int type);
ulong64 rdtsc (void);

void t_start(void);
ulong64 t_read(void);
void init_timer(void);

/* register default algs */
void reg_algs(void);
int time_keysched(void);
int time_cipher(void);
int time_cipher2(void);
int time_cipher3(void);
int time_cipher4(void);
int time_hash(void);
void time_mult(void);
void time_sqr(void);
void time_prng(void);
void time_rsa(void);
void time_dsa(void);
void time_katja(void);
void time_ecc(void);
void time_macs_(unsigned long MAC_SIZE);
void time_macs(void);
void time_encmacs(void);



#if defined(_WIN32)
   #define PRI64  "I64d"
#else
   #define PRI64  "ll"
#endif

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
