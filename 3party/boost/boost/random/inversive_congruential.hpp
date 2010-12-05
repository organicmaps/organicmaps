/* boost random/inversive_congruential.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: inversive_congruential.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_INVERSIVE_CONGRUENTIAL_HPP
#define BOOST_RANDOM_INVERSIVE_CONGRUENTIAL_HPP

#include <iostream>
#include <cassert>
#include <stdexcept>
#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/const_mod.hpp>

namespace boost {
namespace random {

// Eichenauer and Lehn 1986
/**
 * Instantiations of class template @c inversive_congruential model a
 * \pseudo_random_number_generator. It uses the inversive congruential
 * algorithm (ICG) described in
 *
 *  @blockquote
 *  "Inversive pseudorandom number generators: concepts, results and links",
 *  Peter Hellekalek, In: "Proceedings of the 1995 Winter Simulation
 *  Conference", C. Alexopoulos, K. Kang, W.R. Lilegdon, and D. Goldsman
 *  (editors), 1995, pp. 255-262. ftp://random.mat.sbg.ac.at/pub/data/wsc95.ps
 *  @endblockquote
 *
 * The output sequence is defined by x(n+1) = (a*inv(x(n)) - b) (mod p),
 * where x(0), a, b, and the prime number p are parameters of the generator.
 * The expression inv(k) denotes the multiplicative inverse of k in the
 * field of integer numbers modulo p, with inv(0) := 0.
 *
 * The template parameter IntType shall denote a signed integral type large
 * enough to hold p; a, b, and p are the parameters of the generators. The
 * template parameter val is the validation value checked by validation.
 *
 * @xmlnote
 * The implementation currently uses the Euclidian Algorithm to compute
 * the multiplicative inverse. Therefore, the inversive generators are about
 * 10-20 times slower than the others (see section"performance"). However,
 * the paper talks of only 3x slowdown, so the Euclidian Algorithm is probably
 * not optimal for calculating the multiplicative inverse.
 * @endxmlnote
 */
template<class IntType, IntType a, IntType b, IntType p, IntType val>
class inversive_congruential
{
public:
  typedef IntType result_type;
#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
  static const bool has_fixed_range = true;
  static const result_type min_value = (b == 0 ? 1 : 0);
  static const result_type max_value = p-1;
#else
  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
#endif
  BOOST_STATIC_CONSTANT(result_type, multiplier = a);
  BOOST_STATIC_CONSTANT(result_type, increment = b);
  BOOST_STATIC_CONSTANT(result_type, modulus = p);

  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return b == 0 ? 1 : 0; }
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return p-1; }

  /**
   * Constructs an inversive_congruential generator with
   * @c y0 as the initial state.
   */
  explicit inversive_congruential(IntType y0 = 1) : value(y0)
  {
    BOOST_STATIC_ASSERT(b >= 0);
    BOOST_STATIC_ASSERT(p > 1);
    BOOST_STATIC_ASSERT(a >= 1);
    if(b == 0) 
      assert(y0 > 0); 
  }
  template<class It> inversive_congruential(It& first, It last)
  { seed(first, last); }

  /** Changes the current state to y0. */
  void seed(IntType y0 = 1) { value = y0; if(b == 0) assert(y0 > 0); }
  template<class It> void seed(It& first, It last)
  {
    if(first == last)
      throw std::invalid_argument("inversive_congruential::seed");
    value = *first++;
  }
  IntType operator()()
  {
    typedef const_mod<IntType, p> do_mod;
    value = do_mod::mult_add(a, do_mod::invert(value), b);
    return value;
  }

  static bool validation(result_type x) { return val == x; }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, inversive_congruential x)
  { os << x.value; return os; }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, inversive_congruential& x)
  { is >> x.value; return is; }
#endif

  friend bool operator==(inversive_congruential x, inversive_congruential y)
  { return x.value == y.value; }
  friend bool operator!=(inversive_congruential x, inversive_congruential y)
  { return !(x == y); }
#else
  // Use a member function; Streamable concept not supported.
  bool operator==(inversive_congruential rhs) const
  { return value == rhs.value; }
  bool operator!=(inversive_congruential rhs) const
  { return !(*this == rhs); }
#endif
private:
  IntType value;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class IntType, IntType a, IntType b, IntType p, IntType val>
const bool inversive_congruential<IntType, a, b, p, val>::has_fixed_range;
template<class IntType, IntType a, IntType b, IntType p, IntType val>
const typename inversive_congruential<IntType, a, b, p, val>::result_type inversive_congruential<IntType, a, b, p, val>::min_value;
template<class IntType, IntType a, IntType b, IntType p, IntType val>
const typename inversive_congruential<IntType, a, b, p, val>::result_type inversive_congruential<IntType, a, b, p, val>::max_value;
template<class IntType, IntType a, IntType b, IntType p, IntType val>
const typename inversive_congruential<IntType, a, b, p, val>::result_type inversive_congruential<IntType, a, b, p, val>::multiplier;
template<class IntType, IntType a, IntType b, IntType p, IntType val>
const typename inversive_congruential<IntType, a, b, p, val>::result_type inversive_congruential<IntType, a, b, p, val>::increment;
template<class IntType, IntType a, IntType b, IntType p, IntType val>
const typename inversive_congruential<IntType, a, b, p, val>::result_type inversive_congruential<IntType, a, b, p, val>::modulus;
#endif

} // namespace random

/**
 * The specialization hellekalek1995 was suggested in
 *
 *  @blockquote
 *  "Inversive pseudorandom number generators: concepts, results and links",
 *  Peter Hellekalek, In: "Proceedings of the 1995 Winter Simulation
 *  Conference", C. Alexopoulos, K. Kang, W.R. Lilegdon, and D. Goldsman
 *  (editors), 1995, pp. 255-262. ftp://random.mat.sbg.ac.at/pub/data/wsc95.ps
 *  @endblockquote
 */
typedef random::inversive_congruential<int32_t, 9102, 2147483647-36884165,
  2147483647, 0> hellekalek1995;

} // namespace boost

#endif // BOOST_RANDOM_INVERSIVE_CONGRUENTIAL_HPP
