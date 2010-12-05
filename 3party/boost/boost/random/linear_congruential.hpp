/* boost random/linear_congruential.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: linear_congruential.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_LINEAR_CONGRUENTIAL_HPP
#define BOOST_RANDOM_LINEAR_CONGRUENTIAL_HPP

#include <iostream>
#include <cassert>
#include <stdexcept>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/const_mod.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {

/**
 * Instantiations of class template linear_congruential model a
 * \pseudo_random_number_generator. Linear congruential pseudo-random
 * number generators are described in:
 *
 *  "Mathematical methods in large-scale computing units", D. H. Lehmer,
 *  Proc. 2nd Symposium on Large-Scale Digital Calculating Machines,
 *  Harvard University Press, 1951, pp. 141-146
 *
 * Let x(n) denote the sequence of numbers returned by some pseudo-random
 * number generator. Then for the linear congruential generator,
 * x(n+1) := (a * x(n) + c) mod m. Parameters for the generator are
 * x(0), a, c, m. The template parameter IntType shall denote an integral
 * type. It must be large enough to hold values a, c, and m. The template
 * parameters a and c must be smaller than m.
 *
 * Note: The quality of the generator crucially depends on the choice of
 * the parameters. User code should use one of the sensibly parameterized
 * generators such as minstd_rand instead.
 */
template<class IntType, IntType a, IntType c, IntType m, IntType val>
class linear_congruential
{
public:
  typedef IntType result_type;
#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
  static const bool has_fixed_range = true;
  static const result_type min_value = ( c == 0 ? 1 : 0 );
  static const result_type max_value = m-1;
#else
  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
#endif
  BOOST_STATIC_CONSTANT(IntType, multiplier = a);
  BOOST_STATIC_CONSTANT(IntType, increment = c);
  BOOST_STATIC_CONSTANT(IntType, modulus = m);

  // MSVC 6 and possibly others crash when encountering complicated integral
  // constant expressions.  Avoid the check for now.
  // BOOST_STATIC_ASSERT(m == 0 || a < m);
  // BOOST_STATIC_ASSERT(m == 0 || c < m);

  /**
   * Constructs a linear_congruential generator, seeding it with @c x0.
   */
  explicit linear_congruential(IntType x0 = 1)
  { 
    seed(x0);

    // MSVC fails BOOST_STATIC_ASSERT with std::numeric_limits at class scope
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    BOOST_STATIC_ASSERT(std::numeric_limits<IntType>::is_integer);
#endif
  }

  /**
   * Constructs a @c linear_congruential generator and seeds it
   * with values taken from the itrator range [first, last)
   * and adjusts first to point to the element after the last one
   * used.  If there are not enough elements, throws @c std::invalid_argument.
   *
   * first and last must be input iterators.
   */
  template<class It>
  linear_congruential(It& first, It last)
  {
      seed(first, last);
  }

  // compiler-generated copy constructor and assignment operator are fine

  /**
   * If c mod m is zero and x0 mod m is zero, changes the current value of
   * the generator to 1. Otherwise, changes it to x0 mod m. If c is zero,
   * distinct seeds in the range [1,m) will leave the generator in distinct
   * states. If c is not zero, the range is [0,m).
   */
  void seed(IntType x0 = 1)
  {
    // wrap _x if it doesn't fit in the destination
    if(modulus == 0) {
      _x = x0;
    } else {
      _x = x0 % modulus;
    }
    // handle negative seeds
    if(_x <= 0 && _x != 0) {
      _x += modulus;
    }
    // adjust to the correct range
    if(increment == 0 && _x == 0) {
      _x = 1;
    }
    assert(_x >= (min)());
    assert(_x <= (max)());
  }

  /**
   * seeds a @c linear_congruential generator with values taken
   * from the itrator range [first, last) and adjusts @c first to
   * point to the element after the last one used.  If there are
   * not enough elements, throws @c std::invalid_argument.
   *
   * @c first and @c last must be input iterators.
   */
  template<class It>
  void seed(It& first, It last)
  {
    if(first == last)
      throw std::invalid_argument("linear_congruential::seed");
    seed(*first++);
  }

  /**
   * Returns the smallest value that the @c linear_congruential generator
   * can produce.
   */
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return c == 0 ? 1 : 0; }
  /**
   * Returns the largest value that the @c linear_congruential generator
   * can produce.
   */
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return modulus-1; }

  /** Returns the next value of the @c linear_congruential generator. */
  IntType operator()()
  {
    _x = const_mod<IntType, m>::mult_add(a, _x, c);
    return _x;
  }

  static bool validation(IntType x) { return val == x; }

#ifdef BOOST_NO_OPERATORS_IN_NAMESPACE
    
  // Use a member function; Streamable concept not supported.
  bool operator==(const linear_congruential& rhs) const
  { return _x == rhs._x; }
  bool operator!=(const linear_congruential& rhs) const
  { return !(*this == rhs); }

#else 
  friend bool operator==(const linear_congruential& x,
                         const linear_congruential& y)
  { return x._x == y._x; }
  friend bool operator!=(const linear_congruential& x,
                         const linear_congruential& y)
  { return !(x == y); }
    
#if !defined(BOOST_RANDOM_NO_STREAM_OPERATORS) && !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os,
             const linear_congruential& lcg)
  {
    return os << lcg._x;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is,
             linear_congruential& lcg)
  {
    return is >> lcg._x;
  }
 
private:
#endif
#endif

  IntType _x;
};

// probably needs the "no native streams" caveat for STLPort
#if !defined(__SGI_STL_PORT) && BOOST_WORKAROUND(__GNUC__, == 2)
template<class IntType, IntType a, IntType c, IntType m, IntType val>
std::ostream&
operator<<(std::ostream& os,
           const linear_congruential<IntType,a,c,m,val>& lcg)
{
    return os << lcg._x;
}

template<class IntType, IntType a, IntType c, IntType m, IntType val>
std::istream&
operator>>(std::istream& is,
           linear_congruential<IntType,a,c,m,val>& lcg)
{
    return is >> lcg._x;
}
#elif defined(BOOST_RANDOM_NO_STREAM_OPERATORS) || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
template<class CharT, class Traits, class IntType, IntType a, IntType c, IntType m, IntType val>
std::basic_ostream<CharT,Traits>&
operator<<(std::basic_ostream<CharT,Traits>& os,
           const linear_congruential<IntType,a,c,m,val>& lcg)
{
    return os << lcg._x;
}

template<class CharT, class Traits, class IntType, IntType a, IntType c, IntType m, IntType val>
std::basic_istream<CharT,Traits>&
operator>>(std::basic_istream<CharT,Traits>& is,
           linear_congruential<IntType,a,c,m,val>& lcg)
{
    return is >> lcg._x;
}
#endif

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class IntType, IntType a, IntType c, IntType m, IntType val>
const bool linear_congruential<IntType, a, c, m, val>::has_fixed_range;
template<class IntType, IntType a, IntType c, IntType m, IntType val>
const typename linear_congruential<IntType, a, c, m, val>::result_type linear_congruential<IntType, a, c, m, val>::min_value;
template<class IntType, IntType a, IntType c, IntType m, IntType val>
const typename linear_congruential<IntType, a, c, m, val>::result_type linear_congruential<IntType, a, c, m, val>::max_value;
template<class IntType, IntType a, IntType c, IntType m, IntType val>
const IntType linear_congruential<IntType,a,c,m,val>::modulus;
#endif

} // namespace random

// validation values from the publications
/**
 * The specialization \minstd_rand0 was originally suggested in
 *
 *  @blockquote
 *  A pseudo-random number generator for the System/360, P.A. Lewis,
 *  A.S. Goodman, J.M. Miller, IBM Systems Journal, Vol. 8, No. 2,
 *  1969, pp. 136-146
 *  @endblockquote
 *
 * It is examined more closely together with \minstd_rand in
 *
 *  @blockquote
 *  "Random Number Generators: Good ones are hard to find",
 *  Stephen K. Park and Keith W. Miller, Communications of
 *  the ACM, Vol. 31, No. 10, October 1988, pp. 1192-1201 
 *  @endblockquote
 */
typedef random::linear_congruential<int32_t, 16807, 0, 2147483647, 
  1043618065> minstd_rand0;

/** The specialization \minstd_rand was suggested in
 *
 *  @blockquote
 *  "Random Number Generators: Good ones are hard to find",
 *  Stephen K. Park and Keith W. Miller, Communications of
 *  the ACM, Vol. 31, No. 10, October 1988, pp. 1192-1201
 *  @endblockquote
 */
typedef random::linear_congruential<int32_t, 48271, 0, 2147483647,
  399268537> minstd_rand;


#if !defined(BOOST_NO_INT64_T) && !defined(BOOST_NO_INTEGRAL_INT64_T)
/** Class @c rand48 models a \pseudo_random_number_generator. It uses
 * the linear congruential algorithm with the parameters a = 0x5DEECE66D,
 * c = 0xB, m = 2**48. It delivers identical results to the @c lrand48()
 * function available on some systems (assuming lcong48 has not been called).
 *
 * It is only available on systems where @c uint64_t is provided as an
 * integral type, so that for example static in-class constants and/or
 * enum definitions with large @c uint64_t numbers work.
 */
class rand48 
{
public:
  typedef int32_t result_type;
#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
  static const bool has_fixed_range = true;
  static const int32_t min_value = 0;
  static const int32_t max_value = integer_traits<int32_t>::const_max;
#else
  enum { has_fixed_range = false };
#endif
  /**
   * Returns the smallest value that the generator can produce
   */
  int32_t min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return 0; }
  /**
   * Returns the largest value that the generator can produce
   */
  int32_t max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return std::numeric_limits<int32_t>::max BOOST_PREVENT_MACRO_SUBSTITUTION (); }
  
#ifdef BOOST_RANDOM_DOXYGEN
  /**
   * If T is an integral type smaller than int46_t, constructs
   * a \rand48 generator with x(0) := (x0 << 16) | 0x330e.  Otherwise
   * constructs a \rand48 generator with x(0) = x0.
   */
  template<class T> explicit rand48(T x0 = 1);
#else
  rand48() : lcf(cnv(static_cast<int32_t>(1))) {}
  template<class T> explicit rand48(T x0) : lcf(cnv(x0)) { }
#endif
  template<class It> rand48(It& first, It last) : lcf(first, last) { }

  // compiler-generated copy ctor and assignment operator are fine

#ifdef BOOST_RANDOM_DOXYGEN
  /**
   * If T is an integral type smaller than int46_t, changes
   * the current value x(n) of the generator to (x0 << 16) | 0x330e.
   * Otherwise changes the current value x(n) to x0.
   */
  template<class T> void seed(T x0 = 1);
#else
  void seed() { seed(static_cast<int32_t>(1)); }
  template<class T> void seed(T x0) { lcf.seed(cnv(x0)); }
#endif
  template<class It> void seed(It& first, It last) { lcf.seed(first,last); }

  /**
   * Returns the next value of the generator.
   */
  int32_t operator()() { return static_cast<int32_t>(lcf() >> 17); }
  // by experiment from lrand48()
  static bool validation(int32_t x) { return x == 1993516219; }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT,class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const rand48& r)
  { os << r.lcf; return os; }

  template<class CharT,class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, rand48& r)
  { is >> r.lcf; return is; }
#endif

  friend bool operator==(const rand48& x, const rand48& y)
  { return x.lcf == y.lcf; }
  friend bool operator!=(const rand48& x, const rand48& y)
  { return !(x == y); }
#else
  // Use a member function; Streamable concept not supported.
  bool operator==(const rand48& rhs) const
  { return lcf == rhs.lcf; }
  bool operator!=(const rand48& rhs) const
  { return !(*this == rhs); }
#endif
private:
  /// \cond hide_private_members
  random::linear_congruential<uint64_t,
    uint64_t(0xDEECE66DUL) | (uint64_t(0x5) << 32), // xxxxULL is not portable
    0xB, uint64_t(1)<<48, /* unknown */ 0> lcf;
  template<class T>
  static uint64_t cnv(T x) 
  {
    if(sizeof(T) < sizeof(uint64_t)) {
      return (static_cast<uint64_t>(x) << 16) | 0x330e;
    } else {
      return(static_cast<uint64_t>(x));
    }
  }
  static uint64_t cnv(float x) { return(static_cast<uint64_t>(x)); }
  static uint64_t cnv(double x) { return(static_cast<uint64_t>(x)); }
  static uint64_t cnv(long double x) { return(static_cast<uint64_t>(x)); }
  /// \endcond
};
#endif /* !BOOST_NO_INT64_T && !BOOST_NO_INTEGRAL_INT64_T */

} // namespace boost

#include <boost/random/detail/enable_warnings.hpp>

#endif // BOOST_RANDOM_LINEAR_CONGRUENTIAL_HPP
