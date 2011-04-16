// Copyright John Maddock 2008.
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TR1_HPP
#define BOOST_MATH_TR1_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <math.h> // So we can check which std C lib we're using

#ifdef __cplusplus

#include <boost/config.hpp>
#include <boost/static_assert.hpp>

namespace boost{ namespace math{ namespace tr1{ extern "C"{

#else

#define BOOST_PREVENT_MACRO_SUBSTITUTION /**/

#endif // __cplusplus

// we need to import/export our code only if the user has specifically
// asked for it by defining either BOOST_ALL_DYN_LINK if they want all boost
// libraries to be dynamically linked, or BOOST_MATH_TR1_DYN_LINK
// if they want just this one to be dynamically liked:
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_MATH_TR1_DYN_LINK)
// export if this is our own source, otherwise import:
#ifdef BOOST_MATH_TR1_SOURCE
# define BOOST_MATH_TR1_DECL BOOST_SYMBOL_EXPORT
#else
# define BOOST_MATH_TR1_DECL BOOST_SYMBOL_IMPORT
#endif  // BOOST_MATH_TR1_SOURCE
#else
#  define BOOST_MATH_TR1_DECL
#endif  // DYN_LINK
//
// Set any throw specifications on the C99 extern "C" functions - these have to be
// the same as used in the std lib if any.
//
#if defined(__GLIBC__) && defined(__THROW)
#  define BOOST_MATH_C99_THROW_SPEC __THROW
#else
#  define BOOST_MATH_C99_THROW_SPEC
#endif

//
// Now set up the libraries to link against:
//
#if !defined(BOOST_MATH_TR1_NO_LIB) && !defined(BOOST_MATH_TR1_SOURCE) \
   && !defined(BOOST_ALL_NO_LIB) && defined(__cplusplus)
#  define BOOST_LIB_NAME boost_math_c99
#  if defined(BOOST_MATH_TR1_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#     define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
#endif
#if !defined(BOOST_MATH_TR1_NO_LIB) && !defined(BOOST_MATH_TR1_SOURCE) \
   && !defined(BOOST_ALL_NO_LIB) && defined(__cplusplus)
#  define BOOST_LIB_NAME boost_math_c99f
#  if defined(BOOST_MATH_TR1_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#     define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
#endif
#if !defined(BOOST_MATH_TR1_NO_LIB) && !defined(BOOST_MATH_TR1_SOURCE) \
   && !defined(BOOST_ALL_NO_LIB) && defined(__cplusplus) \
   && !defined(BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS)
#  define BOOST_LIB_NAME boost_math_c99l
#  if defined(BOOST_MATH_TR1_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#     define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
#endif
#if !defined(BOOST_MATH_TR1_NO_LIB) && !defined(BOOST_MATH_TR1_SOURCE) \
   && !defined(BOOST_ALL_NO_LIB) && defined(__cplusplus)
#  define BOOST_LIB_NAME boost_math_tr1
#  if defined(BOOST_MATH_TR1_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#     define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
#endif
#if !defined(BOOST_MATH_TR1_NO_LIB) && !defined(BOOST_MATH_TR1_SOURCE) \
   && !defined(BOOST_ALL_NO_LIB) && defined(__cplusplus)
#  define BOOST_LIB_NAME boost_math_tr1f
#  if defined(BOOST_MATH_TR1_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#     define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
#endif
#if !defined(BOOST_MATH_TR1_NO_LIB) && !defined(BOOST_MATH_TR1_SOURCE) \
   && !defined(BOOST_ALL_NO_LIB) && defined(__cplusplus) \
   && !defined(BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS)
#  define BOOST_LIB_NAME boost_math_tr1l
#  if defined(BOOST_MATH_TR1_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#     define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
#endif

#ifndef FLT_EVAL_METHOD
typedef float float_t;
typedef double double_t;
#elif FLT_EVAL_METHOD == 0
typedef float float_t;
typedef double double_t;
#elif FLT_EVAL_METHOD == 1
typedef double float_t;
typedef double double_t;
#else
typedef long double float_t;
typedef long double double_t;
#endif

// C99 Functions:
double BOOST_MATH_TR1_DECL boost_acosh BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_acoshf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_acoshl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_asinh BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_asinhf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_asinhl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_atanh BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_atanhf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_atanhl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_cbrtf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_cbrtl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_copysign BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_copysignf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_copysignl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_erf BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_erff BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_erfl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_erfc BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_erfcf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_erfcl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#if 0
double BOOST_MATH_TR1_DECL boost_exp2 BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_exp2f BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_exp2l BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#endif
double BOOST_MATH_TR1_DECL boost_expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_expm1f BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_expm1l BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#if 0
double BOOST_MATH_TR1_DECL boost_fdim BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_fdimf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_fdiml BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;
double BOOST_MATH_TR1_DECL boost_fma BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y, double z) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_fmaf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y, float z) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_fmal BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y, long double z) BOOST_MATH_C99_THROW_SPEC;
#endif
double BOOST_MATH_TR1_DECL boost_fmax BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_fmaxf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_fmaxl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_fmin BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_fminf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_fminl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_hypot BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_hypotf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_hypotl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;
#if 0
int BOOST_MATH_TR1_DECL boost_ilogb BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
int BOOST_MATH_TR1_DECL boost_ilogbf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
int BOOST_MATH_TR1_DECL boost_ilogbl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#endif
double BOOST_MATH_TR1_DECL boost_lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_lgammaf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_lgammal BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#ifdef BOOST_HAS_LONG_LONG
#if 0
::boost::long_long_type BOOST_MATH_TR1_DECL boost_llrint BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
::boost::long_long_type BOOST_MATH_TR1_DECL boost_llrintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
::boost::long_long_type BOOST_MATH_TR1_DECL boost_llrintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#endif
::boost::long_long_type BOOST_MATH_TR1_DECL boost_llround BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
::boost::long_long_type BOOST_MATH_TR1_DECL boost_llroundf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
::boost::long_long_type BOOST_MATH_TR1_DECL boost_llroundl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#endif
double BOOST_MATH_TR1_DECL boost_log1p BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_log1pf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_log1pl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#if 0
double BOOST_MATH_TR1_DECL log2 BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL log2f BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL log2l BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL logb BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL logbf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL logbl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
long BOOST_MATH_TR1_DECL lrint BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
long BOOST_MATH_TR1_DECL lrintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long BOOST_MATH_TR1_DECL lrintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#endif
long BOOST_MATH_TR1_DECL boost_lround BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
long BOOST_MATH_TR1_DECL boost_lroundf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long BOOST_MATH_TR1_DECL boost_lroundl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#if 0
double BOOST_MATH_TR1_DECL nan BOOST_PREVENT_MACRO_SUBSTITUTION(const char *str) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL nanf BOOST_PREVENT_MACRO_SUBSTITUTION(const char *str) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL nanl BOOST_PREVENT_MACRO_SUBSTITUTION(const char *str) BOOST_MATH_C99_THROW_SPEC;
double BOOST_MATH_TR1_DECL nearbyint BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL nearbyintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL nearbyintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#endif
double BOOST_MATH_TR1_DECL boost_nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_nextafterf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_nextafterl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(double x, long double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_nexttowardf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, long double y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_nexttowardl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;
#if 0
double BOOST_MATH_TR1_DECL boost_remainder BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_remainderf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_remainderl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;
double BOOST_MATH_TR1_DECL boost_remquo BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y, int *pquo) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_remquof BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y, int *pquo) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_remquol BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y, int *pquo) BOOST_MATH_C99_THROW_SPEC;
double BOOST_MATH_TR1_DECL boost_rint BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_rintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_rintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#endif
double BOOST_MATH_TR1_DECL boost_round BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_roundf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_roundl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;
#if 0
double BOOST_MATH_TR1_DECL boost_scalbln BOOST_PREVENT_MACRO_SUBSTITUTION(double x, long ex) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_scalblnf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, long ex) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_scalblnl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long ex) BOOST_MATH_C99_THROW_SPEC;
double BOOST_MATH_TR1_DECL boost_scalbn BOOST_PREVENT_MACRO_SUBSTITUTION(double x, int ex) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_scalbnf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, int ex) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_scalbnl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, int ex) BOOST_MATH_C99_THROW_SPEC;
#endif
double BOOST_MATH_TR1_DECL boost_tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_tgammaf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_tgammal BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

double BOOST_MATH_TR1_DECL boost_trunc BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_truncf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_truncl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.1] associated Laguerre polynomials:
double BOOST_MATH_TR1_DECL boost_assoc_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_assoc_laguerref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_assoc_laguerrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.2] associated Legendre functions:
double BOOST_MATH_TR1_DECL boost_assoc_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_assoc_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_assoc_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.3] beta function:
double BOOST_MATH_TR1_DECL boost_beta BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_betaf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_betal BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.4] (complete) elliptic integral of the first kind:
double BOOST_MATH_TR1_DECL boost_comp_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(double k) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_comp_ellint_1f BOOST_PREVENT_MACRO_SUBSTITUTION(float k) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_comp_ellint_1l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.5] (complete) elliptic integral of the second kind:
double BOOST_MATH_TR1_DECL boost_comp_ellint_2 BOOST_PREVENT_MACRO_SUBSTITUTION(double k) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_comp_ellint_2f BOOST_PREVENT_MACRO_SUBSTITUTION(float k) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_comp_ellint_2l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.6] (complete) elliptic integral of the third kind:
double BOOST_MATH_TR1_DECL boost_comp_ellint_3 BOOST_PREVENT_MACRO_SUBSTITUTION(double k, double nu) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_comp_ellint_3f BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float nu) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_comp_ellint_3l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double nu) BOOST_MATH_C99_THROW_SPEC;
#if 0
// [5.2.1.7] confluent hypergeometric functions:
double BOOST_MATH_TR1_DECL conf_hyperg BOOST_PREVENT_MACRO_SUBSTITUTION(double a, double c, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL conf_hypergf BOOST_PREVENT_MACRO_SUBSTITUTION(float a, float c, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL conf_hypergl BOOST_PREVENT_MACRO_SUBSTITUTION(long double a, long double c, long double x) BOOST_MATH_C99_THROW_SPEC;
#endif
// [5.2.1.8] regular modified cylindrical Bessel functions:
double BOOST_MATH_TR1_DECL boost_cyl_bessel_i BOOST_PREVENT_MACRO_SUBSTITUTION(double nu, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_cyl_bessel_if BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_cyl_bessel_il BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.9] cylindrical Bessel functions (of the first kind):
double BOOST_MATH_TR1_DECL boost_cyl_bessel_j BOOST_PREVENT_MACRO_SUBSTITUTION(double nu, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_cyl_bessel_jf BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_cyl_bessel_jl BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.10] irregular modified cylindrical Bessel functions:
double BOOST_MATH_TR1_DECL boost_cyl_bessel_k BOOST_PREVENT_MACRO_SUBSTITUTION(double nu, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_cyl_bessel_kf BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_cyl_bessel_kl BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.11] cylindrical Neumann functions BOOST_MATH_C99_THROW_SPEC;
// cylindrical Bessel functions (of the second kind):
double BOOST_MATH_TR1_DECL boost_cyl_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(double nu, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_cyl_neumannf BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_cyl_neumannl BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.12] (incomplete) elliptic integral of the first kind:
double BOOST_MATH_TR1_DECL boost_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(double k, double phi) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_ellint_1f BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float phi) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_ellint_1l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double phi) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.13] (incomplete) elliptic integral of the second kind:
double BOOST_MATH_TR1_DECL boost_ellint_2 BOOST_PREVENT_MACRO_SUBSTITUTION(double k, double phi) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_ellint_2f BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float phi) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_ellint_2l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double phi) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.14] (incomplete) elliptic integral of the third kind:
double BOOST_MATH_TR1_DECL boost_ellint_3 BOOST_PREVENT_MACRO_SUBSTITUTION(double k, double nu, double phi) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_ellint_3f BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float nu, float phi) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_ellint_3l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double nu, long double phi) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.15] exponential integral:
double BOOST_MATH_TR1_DECL boost_expint BOOST_PREVENT_MACRO_SUBSTITUTION(double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_expintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_expintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.16] Hermite polynomials:
double BOOST_MATH_TR1_DECL boost_hermite BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_hermitef BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_hermitel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x) BOOST_MATH_C99_THROW_SPEC;

#if 0
// [5.2.1.17] hypergeometric functions:
double BOOST_MATH_TR1_DECL hyperg BOOST_PREVENT_MACRO_SUBSTITUTION(double a, double b, double c, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL hypergf BOOST_PREVENT_MACRO_SUBSTITUTION(float a, float b, float c, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL hypergl BOOST_PREVENT_MACRO_SUBSTITUTION(long double a, long double b, long double c,
long double x) BOOST_MATH_C99_THROW_SPEC;
#endif

// [5.2.1.18] Laguerre polynomials:
double BOOST_MATH_TR1_DECL boost_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_laguerref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_laguerrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.19] Legendre polynomials:
double BOOST_MATH_TR1_DECL boost_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.20] Riemann zeta function:
double BOOST_MATH_TR1_DECL boost_riemann_zeta BOOST_PREVENT_MACRO_SUBSTITUTION(double) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_riemann_zetaf BOOST_PREVENT_MACRO_SUBSTITUTION(float) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_riemann_zetal BOOST_PREVENT_MACRO_SUBSTITUTION(long double) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.21] spherical Bessel functions (of the first kind):
double BOOST_MATH_TR1_DECL boost_sph_bessel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_sph_besself BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_sph_bessell BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.22] spherical associated Legendre functions:
double BOOST_MATH_TR1_DECL boost_sph_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, double theta) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_sph_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, float theta) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_sph_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, long double theta) BOOST_MATH_C99_THROW_SPEC;

// [5.2.1.23] spherical Neumann functions BOOST_MATH_C99_THROW_SPEC;
// spherical Bessel functions (of the second kind):
double BOOST_MATH_TR1_DECL boost_sph_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, double x) BOOST_MATH_C99_THROW_SPEC;
float BOOST_MATH_TR1_DECL boost_sph_neumannf BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x) BOOST_MATH_C99_THROW_SPEC;
long double BOOST_MATH_TR1_DECL boost_sph_neumannl BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x) BOOST_MATH_C99_THROW_SPEC;

#ifdef __cplusplus

}}}}  // namespaces

#include <boost/math/tools/promotion.hpp>

namespace boost{ namespace math{ namespace tr1{
//
// Declare overload of the functions which forward to the
// C interfaces:
//
// C99 Functions:
inline double acosh BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_acosh BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float acoshf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_acoshf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double acoshl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_acoshl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float acosh BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::acoshf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double acosh BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::acoshl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type acosh BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::acosh BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

inline double asinh BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_asinh BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float asinhf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_asinhf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double asinhl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_asinhl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float asinh BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::asinhf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double asinh BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::asinhl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type asinh BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::asinh BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

inline double atanh BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_atanh BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float atanhf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_atanhf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double atanhl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_atanhl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float atanh BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::atanhf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double atanh BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::atanhl(x); }
template <class T>
inline typename tools::promote_args<T>::type atanh BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::atanh BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

inline double cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float cbrtf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_cbrtf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double cbrtl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_cbrtl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::cbrtf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::cbrtl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

inline double copysign BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y)
{ return boost::math::tr1::boost_copysign BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float copysignf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::boost_copysignf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double copysignl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::boost_copysignl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float copysign BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::copysignf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double copysign BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::copysignl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type copysign BOOST_PREVENT_MACRO_SUBSTITUTION(T1 x, T2 y)
{ return boost::math::tr1::copysign BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(x), static_cast<typename tools::promote_args<T1, T2>::type>(y)); }

inline double erf BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_erf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float erff BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_erff BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double erfl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_erfl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float erf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::erff BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double erf BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::erfl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type erf BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::erf BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

inline double erfc BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_erfc BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float erfcf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_erfcf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double erfcl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_erfcl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float erfc BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::erfcf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double erfc BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::erfcl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type erfc BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::erfc BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

#if 0
double exp2 BOOST_PREVENT_MACRO_SUBSTITUTION(double x);
float exp2f BOOST_PREVENT_MACRO_SUBSTITUTION(float x);
long double exp2l BOOST_PREVENT_MACRO_SUBSTITUTION(long double x);
#endif

inline float expm1f BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_expm1f BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline double expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double expm1l BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_expm1l BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::expm1f BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::expm1l BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

#if 0
double fdim BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y);
float fdimf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y);
long double fdiml BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y);
double fma BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y, double z);
float fmaf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y, float z);
long double fmal BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y, long double z);
#endif
inline double fmax BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y)
{ return boost::math::tr1::boost_fmax BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float fmaxf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::boost_fmaxf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double fmaxl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::boost_fmaxl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float fmax BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::fmaxf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double fmax BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::fmaxl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type fmax BOOST_PREVENT_MACRO_SUBSTITUTION(T1 x, T2 y)
{ return boost::math::tr1::fmax BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(x), static_cast<typename tools::promote_args<T1, T2>::type>(y)); }

inline double fmin BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y)
{ return boost::math::tr1::boost_fmin BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float fminf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::boost_fminf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double fminl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::boost_fminl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float fmin BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::fminf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double fmin BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::fminl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type fmin BOOST_PREVENT_MACRO_SUBSTITUTION(T1 x, T2 y)
{ return boost::math::tr1::fmin BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(x), static_cast<typename tools::promote_args<T1, T2>::type>(y)); }

inline float hypotf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::boost_hypotf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline double hypot BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y)
{ return boost::math::tr1::boost_hypot BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double hypotl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::boost_hypotl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float hypot BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::hypotf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double hypot BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::hypotl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type hypot BOOST_PREVENT_MACRO_SUBSTITUTION(T1 x, T2 y)
{ return boost::math::tr1::hypot BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(x), static_cast<typename tools::promote_args<T1, T2>::type>(y)); }

#if 0
int ilogb BOOST_PREVENT_MACRO_SUBSTITUTION(double x);
int ilogbf BOOST_PREVENT_MACRO_SUBSTITUTION(float x);
int ilogbl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x);
#endif

inline float lgammaf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_lgammaf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline double lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double lgammal BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_lgammal BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::lgammaf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::lgammal BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

#ifdef BOOST_HAS_LONG_LONG
#if 0
::boost::long_long_type llrint BOOST_PREVENT_MACRO_SUBSTITUTION(double x);
::boost::long_long_type llrintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x);
::boost::long_long_type llrintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x);
#endif

inline ::boost::long_long_type llroundf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_llroundf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline ::boost::long_long_type llround BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_llround BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline ::boost::long_long_type llroundl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_llroundl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline ::boost::long_long_type llround BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::llroundf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline ::boost::long_long_type llround BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::llroundl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline ::boost::long_long_type llround BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return llround BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<double>(x)); }
#endif

inline float log1pf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_log1pf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline double log1p BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_log1p BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double log1pl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_log1pl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float log1p BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::log1pf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double log1p BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::log1pl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type log1p BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::log1p BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }
#if 0
double log2 BOOST_PREVENT_MACRO_SUBSTITUTION(double x);
float log2f BOOST_PREVENT_MACRO_SUBSTITUTION(float x);
long double log2l BOOST_PREVENT_MACRO_SUBSTITUTION(long double x);

double logb BOOST_PREVENT_MACRO_SUBSTITUTION(double x);
float logbf BOOST_PREVENT_MACRO_SUBSTITUTION(float x);
long double logbl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x);
long lrint BOOST_PREVENT_MACRO_SUBSTITUTION(double x);
long lrintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x);
long lrintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x);
#endif
inline long lroundf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_lroundf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long lround BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_lround BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long lroundl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_lroundl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long lround BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::lroundf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long lround BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::lroundl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
long lround BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::lround BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<double>(x)); }
#if 0
double nan BOOST_PREVENT_MACRO_SUBSTITUTION(const char *str);
float nanf BOOST_PREVENT_MACRO_SUBSTITUTION(const char *str);
long double nanl BOOST_PREVENT_MACRO_SUBSTITUTION(const char *str);
double nearbyint BOOST_PREVENT_MACRO_SUBSTITUTION(double x);
float nearbyintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x);
long double nearbyintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x);
#endif
inline float nextafterf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::boost_nextafterf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline double nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y)
{ return boost::math::tr1::boost_nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double nextafterl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::boost_nextafterl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::nextafterf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::nextafterl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(T1 x, T2 y)
{ return boost::math::tr1::nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(x), static_cast<typename tools::promote_args<T1, T2>::type>(y)); }

inline float nexttowardf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::boost_nexttowardf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline double nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y)
{ return boost::math::tr1::boost_nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double nexttowardl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::boost_nexttowardl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::nexttowardf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::nexttowardl BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(T1 x, T2 y)
{ return boost::math::tr1::nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(x), static_cast<long double>(y)); }
#if 0
double remainder BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y);
float remainderf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y);
long double remainderl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y);
double remquo BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y, int *pquo);
float remquof BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y, int *pquo);
long double remquol BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y, int *pquo);
double rint BOOST_PREVENT_MACRO_SUBSTITUTION(double x);
float rintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x);
long double rintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x);
#endif
inline float roundf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_roundf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline double round BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_round BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double roundl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_roundl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float round BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::roundf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double round BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::roundl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type round BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::round BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }
#if 0
double scalbln BOOST_PREVENT_MACRO_SUBSTITUTION(double x, long ex);
float scalblnf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, long ex);
long double scalblnl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long ex);
double scalbn BOOST_PREVENT_MACRO_SUBSTITUTION(double x, int ex);
float scalbnf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, int ex);
long double scalbnl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, int ex);
#endif
inline float tgammaf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_tgammaf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline double tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double tgammal BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_tgammal BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::tgammaf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::tgammal BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

inline float truncf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_truncf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline double trunc BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_trunc BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double truncl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_truncl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float trunc BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::truncf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double trunc BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::truncl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type trunc BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::trunc BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

# define NO_MACRO_EXPAND /**/
// C99 macros defined as C++ templates
template<class T> bool signbit NO_MACRO_EXPAND(T x)
{ BOOST_STATIC_ASSERT(sizeof(T) == 0); return false; } // must not be instantiated
template<> bool BOOST_MATH_TR1_DECL signbit<float> NO_MACRO_EXPAND(float x);
template<> bool BOOST_MATH_TR1_DECL signbit<double> NO_MACRO_EXPAND(double x);
template<> bool BOOST_MATH_TR1_DECL signbit<long double> NO_MACRO_EXPAND(long double x);

template<class T> int fpclassify NO_MACRO_EXPAND(T x)
{ BOOST_STATIC_ASSERT(sizeof(T) == 0); return false; } // must not be instantiated
template<> int BOOST_MATH_TR1_DECL fpclassify<float> NO_MACRO_EXPAND(float x);
template<> int BOOST_MATH_TR1_DECL fpclassify<double> NO_MACRO_EXPAND(double x);
template<> int BOOST_MATH_TR1_DECL fpclassify<long double> NO_MACRO_EXPAND(long double x);

template<class T> bool isfinite NO_MACRO_EXPAND(T x)
{ BOOST_STATIC_ASSERT(sizeof(T) == 0); return false; } // must not be instantiated
template<> bool BOOST_MATH_TR1_DECL isfinite<float> NO_MACRO_EXPAND(float x);
template<> bool BOOST_MATH_TR1_DECL isfinite<double> NO_MACRO_EXPAND(double x);
template<> bool BOOST_MATH_TR1_DECL isfinite<long double> NO_MACRO_EXPAND(long double x);

template<class T> bool isinf NO_MACRO_EXPAND(T x)
{ BOOST_STATIC_ASSERT(sizeof(T) == 0); return false; } // must not be instantiated
template<> bool BOOST_MATH_TR1_DECL isinf<float> NO_MACRO_EXPAND(float x);
template<> bool BOOST_MATH_TR1_DECL isinf<double> NO_MACRO_EXPAND(double x);
template<> bool BOOST_MATH_TR1_DECL isinf<long double> NO_MACRO_EXPAND(long double x);

template<class T> bool isnan NO_MACRO_EXPAND(T x)
{ BOOST_STATIC_ASSERT(sizeof(T) == 0); return false; } // must not be instantiated
template<> bool BOOST_MATH_TR1_DECL isnan<float> NO_MACRO_EXPAND(float x);
template<> bool BOOST_MATH_TR1_DECL isnan<double> NO_MACRO_EXPAND(double x);
template<> bool BOOST_MATH_TR1_DECL isnan<long double> NO_MACRO_EXPAND(long double x);

template<class T> bool isnormal NO_MACRO_EXPAND(T x)
{ BOOST_STATIC_ASSERT(sizeof(T) == 0); return false; } // must not be instantiated
template<> bool BOOST_MATH_TR1_DECL isnormal<float> NO_MACRO_EXPAND(float x);
template<> bool BOOST_MATH_TR1_DECL isnormal<double> NO_MACRO_EXPAND(double x);
template<> bool BOOST_MATH_TR1_DECL isnormal<long double> NO_MACRO_EXPAND(long double x);

#undef NO_MACRO_EXPAND   
   
// [5.2.1.1] associated Laguerre polynomials:
inline float assoc_laguerref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, float x)
{ return boost::math::tr1::boost_assoc_laguerref BOOST_PREVENT_MACRO_SUBSTITUTION(n, m, x); }
inline double assoc_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, double x)
{ return boost::math::tr1::boost_assoc_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(n, m, x); }
inline long double assoc_laguerrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, long double x)
{ return boost::math::tr1::boost_assoc_laguerrel BOOST_PREVENT_MACRO_SUBSTITUTION(n, m, x); }
inline float assoc_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, float x)
{ return boost::math::tr1::assoc_laguerref BOOST_PREVENT_MACRO_SUBSTITUTION(n, m, x); }
inline long double assoc_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, long double x)
{ return boost::math::tr1::assoc_laguerrel BOOST_PREVENT_MACRO_SUBSTITUTION(n, m, x); }
template <class T> 
inline typename tools::promote_args<T>::type assoc_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, unsigned m, T x)
{ return boost::math::tr1::assoc_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(n, m, static_cast<typename tools::promote_args<T>::type>(x)); }

// [5.2.1.2] associated Legendre functions:
inline float assoc_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, float x)
{ return boost::math::tr1::boost_assoc_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, x); }
inline double assoc_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, double x)
{ return boost::math::tr1::boost_assoc_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, x); }
inline long double assoc_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, long double x)
{ return boost::math::tr1::boost_assoc_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, x); }
inline float assoc_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, float x)
{ return boost::math::tr1::assoc_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, x); }
inline long double assoc_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, long double x)
{ return boost::math::tr1::assoc_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, x); }
template <class T>
inline typename tools::promote_args<T>::type assoc_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, T x)
{ return boost::math::tr1::assoc_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, static_cast<typename tools::promote_args<T>::type>(x)); }

// [5.2.1.3] beta function:
inline float betaf BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::boost_betaf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline double beta BOOST_PREVENT_MACRO_SUBSTITUTION(double x, double y)
{ return boost::math::tr1::boost_beta BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double betal BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::boost_betal BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline float beta BOOST_PREVENT_MACRO_SUBSTITUTION(float x, float y)
{ return boost::math::tr1::betaf BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
inline long double beta BOOST_PREVENT_MACRO_SUBSTITUTION(long double x, long double y)
{ return boost::math::tr1::betal BOOST_PREVENT_MACRO_SUBSTITUTION(x, y); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type beta BOOST_PREVENT_MACRO_SUBSTITUTION(T2 x, T1 y)
{ return boost::math::tr1::beta BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(x), static_cast<typename tools::promote_args<T1, T2>::type>(y)); }

// [5.2.1.4] (complete) elliptic integral of the first kind:
inline float comp_ellint_1f BOOST_PREVENT_MACRO_SUBSTITUTION(float k)
{ return boost::math::tr1::boost_comp_ellint_1f BOOST_PREVENT_MACRO_SUBSTITUTION(k); }
inline double comp_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(double k)
{ return boost::math::tr1::boost_comp_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(k); }
inline long double comp_ellint_1l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k)
{ return boost::math::tr1::boost_comp_ellint_1l BOOST_PREVENT_MACRO_SUBSTITUTION(k); }
inline float comp_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(float k)
{ return boost::math::tr1::comp_ellint_1f BOOST_PREVENT_MACRO_SUBSTITUTION(k); }
inline long double comp_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(long double k)
{ return boost::math::tr1::comp_ellint_1l BOOST_PREVENT_MACRO_SUBSTITUTION(k); }
template <class T>
inline typename tools::promote_args<T>::type comp_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(T k)
{ return boost::math::tr1::comp_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(k)); }

// [5.2.1.5]  BOOST_PREVENT_MACRO_SUBSTITUTION(complete) elliptic integral of the second kind:
inline float comp_ellint_2f(float k)
{ return boost::math::tr1::boost_comp_ellint_2f(k); }
inline double comp_ellint_2(double k)
{ return boost::math::tr1::boost_comp_ellint_2(k); }
inline long double comp_ellint_2l(long double k)
{ return boost::math::tr1::boost_comp_ellint_2l(k); }
inline float comp_ellint_2(float k)
{ return boost::math::tr1::comp_ellint_2f(k); }
inline long double comp_ellint_2(long double k)
{ return boost::math::tr1::comp_ellint_2l(k); }
template <class T>
inline typename tools::promote_args<T>::type comp_ellint_2(T k)
{ return boost::math::tr1::comp_ellint_2(static_cast<typename tools::promote_args<T>::type> BOOST_PREVENT_MACRO_SUBSTITUTION(k)); }

// [5.2.1.6]  BOOST_PREVENT_MACRO_SUBSTITUTION(complete) elliptic integral of the third kind:
inline float comp_ellint_3f(float k, float nu)
{ return boost::math::tr1::boost_comp_ellint_3f(k, nu); }
inline double comp_ellint_3(double k, double nu)
{ return boost::math::tr1::boost_comp_ellint_3(k, nu); }
inline long double comp_ellint_3l(long double k, long double nu)
{ return boost::math::tr1::boost_comp_ellint_3l(k, nu); }
inline float comp_ellint_3(float k, float nu)
{ return boost::math::tr1::comp_ellint_3f(k, nu); }
inline long double comp_ellint_3(long double k, long double nu)
{ return boost::math::tr1::comp_ellint_3l(k, nu); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type comp_ellint_3(T1 k, T2 nu)
{ return boost::math::tr1::comp_ellint_3(static_cast<typename tools::promote_args<T1, T2>::type> BOOST_PREVENT_MACRO_SUBSTITUTION(k), static_cast<typename tools::promote_args<T1, T2>::type> BOOST_PREVENT_MACRO_SUBSTITUTION(nu)); }

#if 0
// [5.2.1.7] confluent hypergeometric functions:
double conf_hyperg BOOST_PREVENT_MACRO_SUBSTITUTION(double a, double c, double x);
float conf_hypergf BOOST_PREVENT_MACRO_SUBSTITUTION(float a, float c, float x);
long double conf_hypergl BOOST_PREVENT_MACRO_SUBSTITUTION(long double a, long double c, long double x);
#endif

// [5.2.1.8] regular modified cylindrical Bessel functions:
inline float cyl_bessel_if BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x)
{ return boost::math::tr1::boost_cyl_bessel_if BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline double cyl_bessel_i BOOST_PREVENT_MACRO_SUBSTITUTION(double nu, double x)
{ return boost::math::tr1::boost_cyl_bessel_i BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline long double cyl_bessel_il BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x)
{ return boost::math::tr1::boost_cyl_bessel_il BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline float cyl_bessel_i BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x)
{ return boost::math::tr1::cyl_bessel_if BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline long double cyl_bessel_i BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x)
{ return boost::math::tr1::cyl_bessel_il BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type cyl_bessel_i BOOST_PREVENT_MACRO_SUBSTITUTION(T1 nu, T2 x)
{ return boost::math::tr1::cyl_bessel_i BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(nu), static_cast<typename tools::promote_args<T1, T2>::type>(x)); }

// [5.2.1.9] cylindrical Bessel functions (of the first kind):
inline float cyl_bessel_jf BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x)
{ return boost::math::tr1::boost_cyl_bessel_jf BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline double cyl_bessel_j BOOST_PREVENT_MACRO_SUBSTITUTION(double nu, double x)
{ return boost::math::tr1::boost_cyl_bessel_j BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline long double cyl_bessel_jl BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x)
{ return boost::math::tr1::boost_cyl_bessel_jl BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline float cyl_bessel_j BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x)
{ return boost::math::tr1::cyl_bessel_jf BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline long double cyl_bessel_j BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x)
{ return boost::math::tr1::cyl_bessel_jl BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type cyl_bessel_j BOOST_PREVENT_MACRO_SUBSTITUTION(T1 nu, T2 x)
{ return boost::math::tr1::cyl_bessel_j BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(nu), static_cast<typename tools::promote_args<T1, T2>::type>(x)); }

// [5.2.1.10] irregular modified cylindrical Bessel functions:
inline float cyl_bessel_kf BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x)
{ return boost::math::tr1::boost_cyl_bessel_kf BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline double cyl_bessel_k BOOST_PREVENT_MACRO_SUBSTITUTION(double nu, double x)
{ return boost::math::tr1::boost_cyl_bessel_k BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline long double cyl_bessel_kl BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x)
{ return boost::math::tr1::boost_cyl_bessel_kl BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline float cyl_bessel_k BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x)
{ return boost::math::tr1::cyl_bessel_kf BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline long double cyl_bessel_k BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x)
{ return boost::math::tr1::cyl_bessel_kl BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type cyl_bessel_k BOOST_PREVENT_MACRO_SUBSTITUTION(T1 nu, T2 x)
{ return boost::math::tr1::cyl_bessel_k BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type> BOOST_PREVENT_MACRO_SUBSTITUTION(nu), static_cast<typename tools::promote_args<T1, T2>::type>(x)); }

// [5.2.1.11] cylindrical Neumann functions;
// cylindrical Bessel functions (of the second kind):
inline float cyl_neumannf BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x)
{ return boost::math::tr1::boost_cyl_neumannf BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline double cyl_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(double nu, double x)
{ return boost::math::tr1::boost_cyl_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline long double cyl_neumannl BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x)
{ return boost::math::tr1::boost_cyl_neumannl BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline float cyl_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(float nu, float x)
{ return boost::math::tr1::cyl_neumannf BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
inline long double cyl_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(long double nu, long double x)
{ return boost::math::tr1::cyl_neumannl BOOST_PREVENT_MACRO_SUBSTITUTION(nu, x); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type cyl_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(T1 nu, T2 x)
{ return boost::math::tr1::cyl_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(nu), static_cast<typename tools::promote_args<T1, T2>::type>(x)); }

// [5.2.1.12] (incomplete) elliptic integral of the first kind:
inline float ellint_1f BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float phi)
{ return boost::math::tr1::boost_ellint_1f BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
inline double ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(double k, double phi)
{ return boost::math::tr1::boost_ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
inline long double ellint_1l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double phi)
{ return boost::math::tr1::boost_ellint_1l BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
inline float ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float phi)
{ return boost::math::tr1::ellint_1f BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
inline long double ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double phi)
{ return boost::math::tr1::ellint_1l BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(T1 k, T2 phi)
{ return boost::math::tr1::ellint_1 BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(k), static_cast<typename tools::promote_args<T1, T2>::type>(phi)); }

// [5.2.1.13] (incomplete) elliptic integral of the second kind:
inline float ellint_2f BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float phi)
{ return boost::math::tr1::boost_ellint_2f BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
inline double ellint_2 BOOST_PREVENT_MACRO_SUBSTITUTION(double k, double phi)
{ return boost::math::tr1::boost_ellint_2 BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
inline long double ellint_2l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double phi)
{ return boost::math::tr1::boost_ellint_2l BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
inline float ellint_2 BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float phi)
{ return boost::math::tr1::ellint_2f BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
inline long double ellint_2 BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double phi)
{ return boost::math::tr1::ellint_2l BOOST_PREVENT_MACRO_SUBSTITUTION(k, phi); }
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type ellint_2 BOOST_PREVENT_MACRO_SUBSTITUTION(T1 k, T2 phi)
{ return boost::math::tr1::ellint_2 BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2>::type>(k), static_cast<typename tools::promote_args<T1, T2>::type>(phi)); }

// [5.2.1.14] (incomplete) elliptic integral of the third kind:
inline float ellint_3f BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float nu, float phi)
{ return boost::math::tr1::boost_ellint_3f BOOST_PREVENT_MACRO_SUBSTITUTION(k, nu, phi); }
inline double ellint_3 BOOST_PREVENT_MACRO_SUBSTITUTION(double k, double nu, double phi)
{ return boost::math::tr1::boost_ellint_3 BOOST_PREVENT_MACRO_SUBSTITUTION(k, nu, phi); }
inline long double ellint_3l BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double nu, long double phi)
{ return boost::math::tr1::boost_ellint_3l BOOST_PREVENT_MACRO_SUBSTITUTION(k, nu, phi); }
inline float ellint_3 BOOST_PREVENT_MACRO_SUBSTITUTION(float k, float nu, float phi)
{ return boost::math::tr1::ellint_3f BOOST_PREVENT_MACRO_SUBSTITUTION(k, nu, phi); }
inline long double ellint_3 BOOST_PREVENT_MACRO_SUBSTITUTION(long double k, long double nu, long double phi)
{ return boost::math::tr1::ellint_3l BOOST_PREVENT_MACRO_SUBSTITUTION(k, nu, phi); }
template <class T1, class T2, class T3>
inline typename tools::promote_args<T1, T2, T3>::type ellint_3 BOOST_PREVENT_MACRO_SUBSTITUTION(T1 k, T2 nu, T3 phi)
{ return boost::math::tr1::ellint_3 BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T1, T2, T3>::type>(k), static_cast<typename tools::promote_args<T1, T2, T3>::type>(nu), static_cast<typename tools::promote_args<T1, T2, T3>::type>(phi)); }

// [5.2.1.15] exponential integral:
inline float expintf BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::boost_expintf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline double expint BOOST_PREVENT_MACRO_SUBSTITUTION(double x)
{ return boost::math::tr1::boost_expint BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double expintl BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::boost_expintl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline float expint BOOST_PREVENT_MACRO_SUBSTITUTION(float x)
{ return boost::math::tr1::expintf BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
inline long double expint BOOST_PREVENT_MACRO_SUBSTITUTION(long double x)
{ return boost::math::tr1::expintl BOOST_PREVENT_MACRO_SUBSTITUTION(x); }
template <class T>
inline typename tools::promote_args<T>::type expint BOOST_PREVENT_MACRO_SUBSTITUTION(T x)
{ return boost::math::tr1::expint BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(x)); }

// [5.2.1.16] Hermite polynomials:
inline float hermitef BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x)
{ return boost::math::tr1::boost_hermitef BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline double hermite BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, double x)
{ return boost::math::tr1::boost_hermite BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline long double hermitel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x)
{ return boost::math::tr1::boost_hermitel BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline float hermite BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x)
{ return boost::math::tr1::hermitef BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline long double hermite BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x)
{ return boost::math::tr1::hermitel BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
template <class T>
inline typename tools::promote_args<T>::type hermite BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, T x)
{ return boost::math::tr1::hermite BOOST_PREVENT_MACRO_SUBSTITUTION(n, static_cast<typename tools::promote_args<T>::type>(x)); }

#if 0
// [5.2.1.17] hypergeometric functions:
double hyperg BOOST_PREVENT_MACRO_SUBSTITUTION(double a, double b, double c, double x);
float hypergf BOOST_PREVENT_MACRO_SUBSTITUTION(float a, float b, float c, float x);
long double hypergl BOOST_PREVENT_MACRO_SUBSTITUTION(long double a, long double b, long double c,
long double x);
#endif

// [5.2.1.18] Laguerre polynomials:
inline float laguerref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x)
{ return boost::math::tr1::boost_laguerref BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline double laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, double x)
{ return boost::math::tr1::boost_laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline long double laguerrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x)
{ return boost::math::tr1::boost_laguerrel BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline float laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x)
{ return boost::math::tr1::laguerref BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline long double laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x)
{ return boost::math::tr1::laguerrel BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
template <class T>
inline typename tools::promote_args<T>::type laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, T x)
{ return boost::math::tr1::laguerre BOOST_PREVENT_MACRO_SUBSTITUTION(n, static_cast<typename tools::promote_args<T>::type>(x)); }

// [5.2.1.19] Legendre polynomials:
inline float legendref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, float x)
{ return boost::math::tr1::boost_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(l, x); }
inline double legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, double x)
{ return boost::math::tr1::boost_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(l, x); }
inline long double legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, long double x)
{ return boost::math::tr1::boost_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(l, x); }
inline float legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, float x)
{ return boost::math::tr1::legendref BOOST_PREVENT_MACRO_SUBSTITUTION(l, x); }
inline long double legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, long double x)
{ return boost::math::tr1::legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(l, x); }
template <class T>
inline typename tools::promote_args<T>::type legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, T x)
{ return boost::math::tr1::legendre BOOST_PREVENT_MACRO_SUBSTITUTION(l, static_cast<typename tools::promote_args<T>::type>(x)); }

// [5.2.1.20] Riemann zeta function:
inline float riemann_zetaf BOOST_PREVENT_MACRO_SUBSTITUTION(float z)
{ return boost::math::tr1::boost_riemann_zetaf BOOST_PREVENT_MACRO_SUBSTITUTION(z); }
inline double riemann_zeta BOOST_PREVENT_MACRO_SUBSTITUTION(double z)
{ return boost::math::tr1::boost_riemann_zeta BOOST_PREVENT_MACRO_SUBSTITUTION(z); }
inline long double riemann_zetal BOOST_PREVENT_MACRO_SUBSTITUTION(long double z)
{ return boost::math::tr1::boost_riemann_zetal BOOST_PREVENT_MACRO_SUBSTITUTION(z); }
inline float riemann_zeta BOOST_PREVENT_MACRO_SUBSTITUTION(float z)
{ return boost::math::tr1::riemann_zetaf BOOST_PREVENT_MACRO_SUBSTITUTION(z); }
inline long double riemann_zeta BOOST_PREVENT_MACRO_SUBSTITUTION(long double z)
{ return boost::math::tr1::riemann_zetal BOOST_PREVENT_MACRO_SUBSTITUTION(z); }
template <class T>
inline typename tools::promote_args<T>::type riemann_zeta BOOST_PREVENT_MACRO_SUBSTITUTION(T z)
{ return boost::math::tr1::riemann_zeta BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<typename tools::promote_args<T>::type>(z)); }

// [5.2.1.21] spherical Bessel functions (of the first kind):
inline float sph_besself BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x)
{ return boost::math::tr1::boost_sph_besself BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline double sph_bessel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, double x)
{ return boost::math::tr1::boost_sph_bessel BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline long double sph_bessell BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x)
{ return boost::math::tr1::boost_sph_bessell BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline float sph_bessel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x)
{ return boost::math::tr1::sph_besself BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline long double sph_bessel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x)
{ return boost::math::tr1::sph_bessell BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
template <class T>
inline typename tools::promote_args<T>::type sph_bessel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, T x)
{ return boost::math::tr1::sph_bessel BOOST_PREVENT_MACRO_SUBSTITUTION(n, static_cast<typename tools::promote_args<T>::type>(x)); }

// [5.2.1.22] spherical associated Legendre functions:
inline float sph_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, float theta)
{ return boost::math::tr1::boost_sph_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, theta); }
inline double sph_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, double theta)
{ return boost::math::tr1::boost_sph_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, theta); }
inline long double sph_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, long double theta)
{ return boost::math::tr1::boost_sph_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, theta); }
inline float sph_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, float theta)
{ return boost::math::tr1::sph_legendref BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, theta); }
inline long double sph_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, long double theta)
{ return boost::math::tr1::sph_legendrel BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, theta); }
template <class T>
inline typename tools::promote_args<T>::type sph_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned l, unsigned m, T theta)
{ return boost::math::tr1::sph_legendre BOOST_PREVENT_MACRO_SUBSTITUTION(l, m, static_cast<typename tools::promote_args<T>::type>(theta)); }

// [5.2.1.23] spherical Neumann functions;
// spherical Bessel functions (of the second kind):
inline float sph_neumannf BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x)
{ return boost::math::tr1::boost_sph_neumannf BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline double sph_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, double x)
{ return boost::math::tr1::boost_sph_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline long double sph_neumannl BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x)
{ return boost::math::tr1::boost_sph_neumannl BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline float sph_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, float x)
{ return boost::math::tr1::sph_neumannf BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
inline long double sph_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, long double x)
{ return boost::math::tr1::sph_neumannl BOOST_PREVENT_MACRO_SUBSTITUTION(n, x); }
template <class T>
inline typename tools::promote_args<T>::type sph_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(unsigned n, T x)
{ return boost::math::tr1::sph_neumann BOOST_PREVENT_MACRO_SUBSTITUTION(n, static_cast<typename tools::promote_args<T>::type>(x)); }

}}} // namespaces

#else // __cplusplus

// C99 Functions:
#ifdef acosh
#undef acosh
#endif
#define acosh boost_acosh
#ifdef acoshf
#undef acoshf
#endif
#define acoshf boost_acoshf
#ifdef acoshl
#undef acoshl
#endif
#define acoshl boost_acoshl

#ifdef asinh
#undef asinh
#endif
#define asinh boost_asinh
#ifdef asinhf
#undef asinhf
#endif
#define asinhf boost_asinhf
#ifdef asinhl
#undef asinhl
#endif
#define asinhl boost_asinhl

#ifdef atanh
#undef atanh
#endif
#define atanh boost_atanh
#ifdef atanhf
#undef atanhf
#endif
#define atanhf boost_atanhf
#ifdef atanhl
#undef atanhl
#endif
#define atanhl boost_atanhl

#ifdef cbrt
#undef cbrt
#endif
#define cbrt boost_cbrt
#ifdef cbrtf
#undef cbrtf
#endif
#define cbrtf boost_cbrtf
#ifdef cbrtl
#undef cbrtl
#endif
#define cbrtl boost_cbrtl

#ifdef copysign
#undef copysign
#endif
#define copysign boost_copysign
#ifdef copysignf
#undef copysignf
#endif
#define copysignf boost_copysignf
#ifdef copysignl
#undef copysignl
#endif
#define copysignl boost_copysignl

#ifdef erf
#undef erf
#endif
#define erf boost_erf
#ifdef erff
#undef erff
#endif
#define erff boost_erff
#ifdef erfl
#undef erfl
#endif
#define erfl boost_erfl

#ifdef erfc
#undef erfc
#endif
#define erfc boost_erfc
#ifdef erfcf
#undef erfcf
#endif
#define erfcf boost_erfcf
#ifdef erfcl
#undef erfcl
#endif
#define erfcl boost_erfcl

#if 0
#ifdef exp2
#undef exp2
#endif
#define exp2 boost_exp2
#ifdef exp2f
#undef exp2f
#endif
#define exp2f boost_exp2f
#ifdef exp2l
#undef exp2l
#endif
#define exp2l boost_exp2l
#endif

#ifdef expm1
#undef expm1
#endif
#define expm1 boost_expm1
#ifdef expm1f
#undef expm1f
#endif
#define expm1f boost_expm1f
#ifdef expm1l
#undef expm1l
#endif
#define expm1l boost_expm1l

#if 0
#ifdef fdim
#undef fdim
#endif
#define fdim boost_fdim
#ifdef fdimf
#undef fdimf
#endif
#define fdimf boost_fdimf
#ifdef fdiml
#undef fdiml
#endif
#define fdiml boost_fdiml
#ifdef acosh
#undef acosh
#endif
#define fma boost_fma
#ifdef fmaf
#undef fmaf
#endif
#define fmaf boost_fmaf
#ifdef fmal
#undef fmal
#endif
#define fmal boost_fmal
#endif

#ifdef fmax
#undef fmax
#endif
#define fmax boost_fmax
#ifdef fmaxf
#undef fmaxf
#endif
#define fmaxf boost_fmaxf
#ifdef fmaxl
#undef fmaxl
#endif
#define fmaxl boost_fmaxl

#ifdef fmin
#undef fmin
#endif
#define fmin boost_fmin
#ifdef fminf
#undef fminf
#endif
#define fminf boost_fminf
#ifdef fminl
#undef fminl
#endif
#define fminl boost_fminl

#ifdef hypot
#undef hypot
#endif
#define hypot boost_hypot
#ifdef hypotf
#undef hypotf
#endif
#define hypotf boost_hypotf
#ifdef hypotl
#undef hypotl
#endif
#define hypotl boost_hypotl

#if 0
#ifdef ilogb
#undef ilogb
#endif
#define ilogb boost_ilogb
#ifdef ilogbf
#undef ilogbf
#endif
#define ilogbf boost_ilogbf
#ifdef ilogbl
#undef ilogbl
#endif
#define ilogbl boost_ilogbl
#endif

#ifdef lgamma
#undef lgamma
#endif
#define lgamma boost_lgamma
#ifdef lgammaf
#undef lgammaf
#endif
#define lgammaf boost_lgammaf
#ifdef lgammal
#undef lgammal
#endif
#define lgammal boost_lgammal

#ifdef BOOST_HAS_LONG_LONG
#if 0
#ifdef llrint
#undef llrint
#endif
#define llrint boost_llrint
#ifdef llrintf
#undef llrintf
#endif
#define llrintf boost_llrintf
#ifdef llrintl
#undef llrintl
#endif
#define llrintl boost_llrintl
#endif
#ifdef llround
#undef llround
#endif
#define llround boost_llround
#ifdef llroundf
#undef llroundf
#endif
#define llroundf boost_llroundf
#ifdef llroundl
#undef llroundl
#endif
#define llroundl boost_llroundl
#endif

#ifdef log1p
#undef log1p
#endif
#define log1p boost_log1p
#ifdef log1pf
#undef log1pf
#endif
#define log1pf boost_log1pf
#ifdef log1pl
#undef log1pl
#endif
#define log1pl boost_log1pl

#if 0
#ifdef log2
#undef log2
#endif
#define log2 boost_log2
#ifdef log2f
#undef log2f
#endif
#define log2f boost_log2f
#ifdef log2l
#undef log2l
#endif
#define log2l boost_log2l

#ifdef logb
#undef logb
#endif
#define logb boost_logb
#ifdef logbf
#undef logbf
#endif
#define logbf boost_logbf
#ifdef logbl
#undef logbl
#endif
#define logbl boost_logbl

#ifdef lrint
#undef lrint
#endif
#define lrint boost_lrint
#ifdef lrintf
#undef lrintf
#endif
#define lrintf boost_lrintf
#ifdef lrintl
#undef lrintl
#endif
#define lrintl boost_lrintl
#endif

#ifdef lround
#undef lround
#endif
#define lround boost_lround
#ifdef lroundf
#undef lroundf
#endif
#define lroundf boost_lroundf
#ifdef lroundl
#undef lroundl
#endif
#define lroundl boost_lroundl

#if 0
#ifdef nan
#undef nan
#endif
#define nan boost_nan
#ifdef nanf
#undef nanf
#endif
#define nanf boost_nanf
#ifdef nanl
#undef nanl
#endif
#define nanl boost_nanl

#ifdef nearbyint
#undef nearbyint
#endif
#define nearbyint boost_nearbyint
#ifdef nearbyintf
#undef nearbyintf
#endif
#define nearbyintf boost_nearbyintf
#ifdef nearbyintl
#undef nearbyintl
#endif
#define nearbyintl boost_nearbyintl
#endif

#ifdef nextafter
#undef nextafter
#endif
#define nextafter boost_nextafter
#ifdef nextafterf
#undef nextafterf
#endif
#define nextafterf boost_nextafterf
#ifdef nextafterl
#undef nextafterl
#endif
#define nextafterl boost_nextafterl

#ifdef nexttoward
#undef nexttoward
#endif
#define nexttoward boost_nexttoward
#ifdef nexttowardf
#undef nexttowardf
#endif
#define nexttowardf boost_nexttowardf
#ifdef nexttowardl
#undef nexttowardl
#endif
#define nexttowardl boost_nexttowardl

#if 0
#ifdef remainder
#undef remainder
#endif
#define remainder boost_remainder
#ifdef remainderf
#undef remainderf
#endif
#define remainderf boost_remainderf
#ifdef remainderl
#undef remainderl
#endif
#define remainderl boost_remainderl

#ifdef remquo
#undef remquo
#endif
#define remquo boost_remquo
#ifdef remquof
#undef remquof
#endif
#define remquof boost_remquof
#ifdef remquol
#undef remquol
#endif
#define remquol boost_remquol

#ifdef rint
#undef rint
#endif
#define rint boost_rint
#ifdef rintf
#undef rintf
#endif
#define rintf boost_rintf
#ifdef rintl
#undef rintl
#endif
#define rintl boost_rintl
#endif

#ifdef round
#undef round
#endif
#define round boost_round
#ifdef roundf
#undef roundf
#endif
#define roundf boost_roundf
#ifdef roundl
#undef roundl
#endif
#define roundl boost_roundl

#if 0
#ifdef scalbln
#undef scalbln
#endif
#define scalbln boost_scalbln
#ifdef scalblnf
#undef scalblnf
#endif
#define scalblnf boost_scalblnf
#ifdef scalblnl
#undef scalblnl
#endif
#define scalblnl boost_scalblnl

#ifdef scalbn
#undef scalbn
#endif
#define scalbn boost_scalbn
#ifdef scalbnf
#undef scalbnf
#endif
#define scalbnf boost_scalbnf
#ifdef scalbnl
#undef scalbnl
#endif
#define scalbnl boost_scalbnl
#endif

#ifdef tgamma
#undef tgamma
#endif
#define tgamma boost_tgamma
#ifdef tgammaf
#undef tgammaf
#endif
#define tgammaf boost_tgammaf
#ifdef tgammal
#undef tgammal
#endif
#define tgammal boost_tgammal

#ifdef trunc
#undef trunc
#endif
#define trunc boost_trunc
#ifdef truncf
#undef truncf
#endif
#define truncf boost_truncf
#ifdef truncl
#undef truncl
#endif
#define truncl boost_truncl

// [5.2.1.1] associated Laguerre polynomials:
#ifdef assoc_laguerre
#undef assoc_laguerre
#endif
#define assoc_laguerre boost_assoc_laguerre
#ifdef assoc_laguerref
#undef assoc_laguerref
#endif
#define assoc_laguerref boost_assoc_laguerref
#ifdef assoc_laguerrel
#undef assoc_laguerrel
#endif
#define assoc_laguerrel boost_assoc_laguerrel

// [5.2.1.2] associated Legendre functions:
#ifdef assoc_legendre
#undef assoc_legendre
#endif
#define assoc_legendre boost_assoc_legendre
#ifdef assoc_legendref
#undef assoc_legendref
#endif
#define assoc_legendref boost_assoc_legendref
#ifdef assoc_legendrel
#undef assoc_legendrel
#endif
#define assoc_legendrel boost_assoc_legendrel

// [5.2.1.3] beta function:
#ifdef beta
#undef beta
#endif
#define beta boost_beta
#ifdef betaf
#undef betaf
#endif
#define betaf boost_betaf
#ifdef betal
#undef betal
#endif
#define betal boost_betal

// [5.2.1.4] (complete) elliptic integral of the first kind:
#ifdef comp_ellint_1
#undef comp_ellint_1
#endif
#define comp_ellint_1 boost_comp_ellint_1
#ifdef comp_ellint_1f
#undef comp_ellint_1f
#endif
#define comp_ellint_1f boost_comp_ellint_1f
#ifdef comp_ellint_1l
#undef comp_ellint_1l
#endif
#define comp_ellint_1l boost_comp_ellint_1l

// [5.2.1.5] (complete) elliptic integral of the second kind:
#ifdef comp_ellint_2
#undef comp_ellint_2
#endif
#define comp_ellint_2 boost_comp_ellint_2
#ifdef comp_ellint_2f
#undef comp_ellint_2f
#endif
#define comp_ellint_2f boost_comp_ellint_2f
#ifdef comp_ellint_2l
#undef comp_ellint_2l
#endif
#define comp_ellint_2l boost_comp_ellint_2l

// [5.2.1.6] (complete) elliptic integral of the third kind:
#ifdef comp_ellint_3
#undef comp_ellint_3
#endif
#define comp_ellint_3 boost_comp_ellint_3
#ifdef comp_ellint_3f
#undef comp_ellint_3f
#endif
#define comp_ellint_3f boost_comp_ellint_3f
#ifdef comp_ellint_3l
#undef comp_ellint_3l
#endif
#define comp_ellint_3l boost_comp_ellint_3l

#if 0
// [5.2.1.7] confluent hypergeometric functions:
#ifdef conf_hyper
#undef conf_hyper
#endif
#define conf_hyper boost_conf_hyper
#ifdef conf_hyperf
#undef conf_hyperf
#endif
#define conf_hyperf boost_conf_hyperf
#ifdef conf_hyperl
#undef conf_hyperl
#endif
#define conf_hyperl boost_conf_hyperl
#endif

// [5.2.1.8] regular modified cylindrical Bessel functions:
#ifdef cyl_bessel_i
#undef cyl_bessel_i
#endif
#define cyl_bessel_i boost_cyl_bessel_i
#ifdef cyl_bessel_if
#undef cyl_bessel_if
#endif
#define cyl_bessel_if boost_cyl_bessel_if
#ifdef cyl_bessel_il
#undef cyl_bessel_il
#endif
#define cyl_bessel_il boost_cyl_bessel_il

// [5.2.1.9] cylindrical Bessel functions (of the first kind):
#ifdef cyl_bessel_j
#undef cyl_bessel_j
#endif
#define cyl_bessel_j boost_cyl_bessel_j
#ifdef cyl_bessel_jf
#undef cyl_bessel_jf
#endif
#define cyl_bessel_jf boost_cyl_bessel_jf
#ifdef cyl_bessel_jl
#undef cyl_bessel_jl
#endif
#define cyl_bessel_jl boost_cyl_bessel_jl

// [5.2.1.10] irregular modified cylindrical Bessel functions:
#ifdef cyl_bessel_k
#undef cyl_bessel_k
#endif
#define cyl_bessel_k boost_cyl_bessel_k
#ifdef cyl_bessel_kf
#undef cyl_bessel_kf
#endif
#define cyl_bessel_kf boost_cyl_bessel_kf
#ifdef cyl_bessel_kl
#undef cyl_bessel_kl
#endif
#define cyl_bessel_kl boost_cyl_bessel_kl

// [5.2.1.11] cylindrical Neumann functions BOOST_MATH_C99_THROW_SPEC;
// cylindrical Bessel functions (of the second kind):
#ifdef cyl_neumann
#undef cyl_neumann
#endif
#define cyl_neumann boost_cyl_neumann
#ifdef cyl_neumannf
#undef cyl_neumannf
#endif
#define cyl_neumannf boost_cyl_neumannf
#ifdef cyl_neumannl
#undef cyl_neumannl
#endif
#define cyl_neumannl boost_cyl_neumannl

// [5.2.1.12] (incomplete) elliptic integral of the first kind:
#ifdef ellint_1
#undef ellint_1
#endif
#define ellint_1 boost_ellint_1
#ifdef ellint_1f
#undef ellint_1f
#endif
#define ellint_1f boost_ellint_1f
#ifdef ellint_1l
#undef ellint_1l
#endif
#define ellint_1l boost_ellint_1l

// [5.2.1.13] (incomplete) elliptic integral of the second kind:
#ifdef ellint_2
#undef ellint_2
#endif
#define ellint_2 boost_ellint_2
#ifdef ellint_2f
#undef ellint_2f
#endif
#define ellint_2f boost_ellint_2f
#ifdef ellint_2l
#undef ellint_2l
#endif
#define ellint_2l boost_ellint_2l

// [5.2.1.14] (incomplete) elliptic integral of the third kind:
#ifdef ellint_3
#undef ellint_3
#endif
#define ellint_3 boost_ellint_3
#ifdef ellint_3f
#undef ellint_3f
#endif
#define ellint_3f boost_ellint_3f
#ifdef ellint_3l
#undef ellint_3l
#endif
#define ellint_3l boost_ellint_3l

// [5.2.1.15] exponential integral:
#ifdef expint
#undef expint
#endif
#define expint boost_expint
#ifdef expintf
#undef expintf
#endif
#define expintf boost_expintf
#ifdef expintl
#undef expintl
#endif
#define expintl boost_expintl

// [5.2.1.16] Hermite polynomials:
#ifdef hermite
#undef hermite
#endif
#define hermite boost_hermite
#ifdef hermitef
#undef hermitef
#endif
#define hermitef boost_hermitef
#ifdef hermitel
#undef hermitel
#endif
#define hermitel boost_hermitel

#if 0
// [5.2.1.17] hypergeometric functions:
#ifdef hyperg
#undef hyperg
#endif
#define hyperg boost_hyperg
#ifdef hypergf
#undef hypergf
#endif
#define hypergf boost_hypergf
#ifdef hypergl
#undef hypergl
#endif
#define hypergl boost_hypergl
#endif

// [5.2.1.18] Laguerre polynomials:
#ifdef laguerre
#undef laguerre
#endif
#define laguerre boost_laguerre
#ifdef laguerref
#undef laguerref
#endif
#define laguerref boost_laguerref
#ifdef laguerrel
#undef laguerrel
#endif
#define laguerrel boost_laguerrel

// [5.2.1.19] Legendre polynomials:
#ifdef legendre
#undef legendre
#endif
#define legendre boost_legendre
#ifdef legendref
#undef legendref
#endif
#define legendref boost_legendref
#ifdef legendrel
#undef legendrel
#endif
#define legendrel boost_legendrel

// [5.2.1.20] Riemann zeta function:
#ifdef riemann_zeta
#undef riemann_zeta
#endif
#define riemann_zeta boost_riemann_zeta
#ifdef riemann_zetaf
#undef riemann_zetaf
#endif
#define riemann_zetaf boost_riemann_zetaf
#ifdef riemann_zetal
#undef riemann_zetal
#endif
#define riemann_zetal boost_riemann_zetal

// [5.2.1.21] spherical Bessel functions (of the first kind):
#ifdef sph_bessel
#undef sph_bessel
#endif
#define sph_bessel boost_sph_bessel
#ifdef sph_besself
#undef sph_besself
#endif
#define sph_besself boost_sph_besself
#ifdef sph_bessell
#undef sph_bessell
#endif
#define sph_bessell boost_sph_bessell

// [5.2.1.22] spherical associated Legendre functions:
#ifdef sph_legendre
#undef sph_legendre
#endif
#define sph_legendre boost_sph_legendre
#ifdef sph_legendref
#undef sph_legendref
#endif
#define sph_legendref boost_sph_legendref
#ifdef sph_legendrel
#undef sph_legendrel
#endif
#define sph_legendrel boost_sph_legendrel

// [5.2.1.23] spherical Neumann functions BOOST_MATH_C99_THROW_SPEC;
// spherical Bessel functions (of the second kind):
#ifdef sph_neumann
#undef sph_neumann
#endif
#define sph_neumann boost_sph_neumann
#ifdef sph_neumannf
#undef sph_neumannf
#endif
#define sph_neumannf boost_sph_neumannf
#ifdef sph_neumannl
#undef sph_neumannl
#endif
#define sph_neumannl boost_sph_neumannl

#endif // __cplusplus

#endif // BOOST_MATH_TR1_HPP

