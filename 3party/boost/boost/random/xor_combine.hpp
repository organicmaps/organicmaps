/* boost random/xor_combine.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: xor_combine.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 */

#ifndef BOOST_RANDOM_XOR_COMBINE_HPP
#define BOOST_RANDOM_XOR_COMBINE_HPP

#include <iostream>
#include <cassert>
#include <algorithm> // for std::min and std::max
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>     // uint32_t
#include <boost/random/detail/config.hpp>


namespace boost {
namespace random {

/// \cond hide_private_members
#ifndef BOOST_NO_DEPENDENT_TYPES_IN_TEMPLATE_VALUE_PARAMETERS
  #define BOOST_RANDOM_VAL_TYPE typename URNG1::result_type 
#else
  #define BOOST_RANDOM_VAL_TYPE uint32_t
#endif
/// \endcond

/**
 * Instantiations of @c xor_combine model a \pseudo_random_number_generator.
 * To produce its output it invokes each of the base generators, shifts
 * their results and xors them together.
 */
template<class URNG1, int s1, class URNG2, int s2
#ifndef BOOST_RANDOM_DOXYGEN
, BOOST_RANDOM_VAL_TYPE val = 0
#endif
>
class xor_combine
{
public:
  typedef URNG1 base1_type;
  typedef URNG2 base2_type;
  typedef typename base1_type::result_type result_type;

  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
  BOOST_STATIC_CONSTANT(int, shift1 = s1);
  BOOST_STATIC_CONSTANT(int, shift2 = s2);

  /**
   * Constructors a @c xor_combine by default constructing
   * both base generators.
   */
  xor_combine() : _rng1(), _rng2()
  { }
  /**
   * Constructs a @c xor_combine by copying two base generators.
   */
  xor_combine(const base1_type & rng1, const base2_type & rng2)
    : _rng1(rng1), _rng2(rng2) { }
  /**
   * Constructs a @c xor_combine, seeding both base generators
   * with @c v.
   */
  xor_combine(const result_type & v)
    : _rng1(v), _rng2(v) { }
  /**
   * Constructs a @c xor_combine, seeding both base generators
   * with values from the iterator range [first, last) and changes
   * first to point to the element after the last one used.  If there
   * are not enough elements in the range to seed both generators,
   * throws @c std::invalid_argument.
   */
  template<class It> xor_combine(It& first, It last)
    : _rng1(first, last), _rng2( /* advanced by other call */ first, last) { }
  /**
   * Calls @c seed() for both base generators.
   */
  void seed() { _rng1.seed(); _rng2.seed(); }
  /**
   * @c seeds both base generators with @c v.
   */
  void seed(const result_type & v) { _rng1.seed(v); _rng2.seed(v); }
  /**
   * seeds both base generators with values from the iterator
   * range [first, last) and changes first to point to the element
   * after the last one used.  If there are not enough elements in
   * the range to seed both generators, throws @c std::invalid_argument.
   */
  template<class It> void seed(It& first, It last)
  {
    _rng1.seed(first, last);
    _rng2.seed(first, last);
  }

  /** Returns the first base generator. */
  const base1_type& base1() { return _rng1; }
  /** Returns the second base generator. */
  const base2_type& base2() { return _rng2; }

  /** Returns the next value of the generator. */
  result_type operator()()
  {
    // MSVC fails BOOST_STATIC_ASSERT with std::numeric_limits at class scope
#if !defined(BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS) && !(defined(BOOST_MSVC) && BOOST_MSVC <= 1300)
    BOOST_STATIC_ASSERT(std::numeric_limits<typename base1_type::result_type>::is_integer);
    BOOST_STATIC_ASSERT(std::numeric_limits<typename base2_type::result_type>::is_integer);
    BOOST_STATIC_ASSERT(std::numeric_limits<typename base1_type::result_type>::digits >= std::numeric_limits<typename base2_type::result_type>::digits);
#endif
    return (_rng1() << s1) ^ (_rng2() << s2);
  }

  /**
   * Returns the smallest value that the generator can produce.
   */
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return std::min BOOST_PREVENT_MACRO_SUBSTITUTION((_rng1.min)(), (_rng2.min)()); }
  /**
   * Returns the largest value that the generator can produce.
   */
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return std::max BOOST_PREVENT_MACRO_SUBSTITUTION((_rng1.min)(), (_rng2.max)()); }
  static bool validation(result_type x) { return val == x; }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const xor_combine& s)
  {
    os << s._rng1 << " " << s._rng2 << " ";
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, xor_combine& s)
  {
    is >> s._rng1 >> std::ws >> s._rng2 >> std::ws;
    return is;
  }
#endif

  friend bool operator==(const xor_combine& x, const xor_combine& y)
  { return x._rng1 == y._rng1 && x._rng2 == y._rng2; }
  friend bool operator!=(const xor_combine& x, const xor_combine& y)
  { return !(x == y); }
#else
  // Use a member function; Streamable concept not supported.
  bool operator==(const xor_combine& rhs) const
  { return _rng1 == rhs._rng1 && _rng2 == rhs._rng2; }
  bool operator!=(const xor_combine& rhs) const
  { return !(*this == rhs); }
#endif

private:
  base1_type _rng1;
  base2_type _rng2;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class URNG1, int s1, class URNG2, int s2, BOOST_RANDOM_VAL_TYPE val>
const bool xor_combine<URNG1, s1, URNG2, s2, val>::has_fixed_range;
template<class URNG1, int s1, class URNG2, int s2, BOOST_RANDOM_VAL_TYPE val>
const int xor_combine<URNG1, s1, URNG2, s2, val>::shift1;
template<class URNG1, int s1, class URNG2, int s2, BOOST_RANDOM_VAL_TYPE val>
const int xor_combine<URNG1, s1, URNG2, s2, val>::shift2;
#endif

#undef BOOST_RANDOM_VAL_TYPE

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_XOR_COMBINE_HPP
