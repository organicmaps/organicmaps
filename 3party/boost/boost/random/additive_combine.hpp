/* boost random/additive_combine.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: additive_combine.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_ADDITIVE_COMBINE_HPP
#define BOOST_RANDOM_ADDITIVE_COMBINE_HPP

#include <iostream>
#include <algorithm> // for std::min and std::max
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/linear_congruential.hpp>

namespace boost {
namespace random {

/**
 * An instantiation of class template \additive_combine model a
 * \pseudo_random_number_generator. It combines two multiplicative
 * \linear_congruential number generators, i.e. those with @c c = 0.
 * It is described in
 *
 *  @blockquote
 *  "Efficient and Portable Combined Random Number Generators", Pierre L'Ecuyer,
 *  Communications of the ACM, Vol. 31, No. 6, June 1988, pp. 742-749, 774
 *  @endblockquote
 *
 * The template parameters MLCG1 and MLCG2 shall denote two different
 * \linear_congruential number generators, each with c = 0. Each invocation
 * returns a random number X(n) := (MLCG1(n) - MLCG2(n)) mod (m1 - 1), where
 * m1 denotes the modulus of MLCG1. 
 *
 * The template parameter @c val is the validation value checked by validation.
 */
template<class MLCG1, class MLCG2,
#ifndef BOOST_NO_DEPENDENT_TYPES_IN_TEMPLATE_VALUE_PARAMETERS
  typename MLCG1::result_type 
#else
  int32_t
#endif
  val>
class additive_combine
{
public:
  typedef MLCG1 first_base;
  typedef MLCG2 second_base;
  typedef typename MLCG1::result_type result_type;
#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
  static const bool has_fixed_range = true;
  static const result_type min_value = 1;
  static const result_type max_value = MLCG1::max_value-1;
#else
  enum { has_fixed_range = false };
#endif
  /**
   * Returns: The smallest value that the generator can produce
   */
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return 1; }
  /**
   * Returns: The largest value that the generator can produce
   */
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return (_mlcg1.max)()-1; }

  /**
   * Constructs an \additive_combine generator using the
   * default constructors of the two base generators.
   */
  additive_combine() : _mlcg1(), _mlcg2() { }
  /**
   * Constructs an \additive_combine generator, using aseed as
   * the constructor argument for both base generators.
   */
  explicit additive_combine(result_type aseed)
    : _mlcg1(aseed), _mlcg2(aseed) { }
  /**
   * Constructs an \additive_combine generator, using
   * @c seed1 and @c seed2 as the constructor argument to
   * the first and second base generators, respectively.
   */
  additive_combine(typename MLCG1::result_type seed1, 
                   typename MLCG2::result_type seed2)
    : _mlcg1(seed1), _mlcg2(seed2) { }
  /**
   * Contructs an \additive_combine generator with
   * values from the range defined by the input iterators first
   * and last.  first will be modified to point to the element
   * after the last one used.
   *
   * Throws: @c std::invalid_argument if the input range is too small.
   *
   * Exception Safety: Basic
   */
  template<class It> additive_combine(It& first, It last)
    : _mlcg1(first, last), _mlcg2(first, last) { }

  /**
   * Seeds an \additive_combine generator using the default
   * seeds of the two base generators.
   */
  void seed()
  {
    _mlcg1.seed();
    _mlcg2.seed();
  }

  /**
   * Seeds an \additive_combine generator, using @c aseed as the
   * seed for both base generators.
   */
  void seed(result_type aseed)
  {
    _mlcg1.seed(aseed);
    _mlcg2.seed(aseed);
  }

  /**
   * Seeds an \additive_combine generator, using @c seed1 and @c seed2 as
   * the seeds to the first and second base generators, respectively.
   */
  void seed(typename MLCG1::result_type seed1,
            typename MLCG2::result_type seed2)
  {
    _mlcg1.seed(seed1);
    _mlcg2.seed(seed2);
  }

  /**
   * Seeds an \additive_combine generator with
   * values from the range defined by the input iterators first
   * and last.  first will be modified to point to the element
   * after the last one used.
   *
   * Throws: @c std::invalid_argument if the input range is too small.
   *
   * Exception Safety: Basic
   */
  template<class It> void seed(It& first, It last)
  {
    _mlcg1.seed(first, last);
    _mlcg2.seed(first, last);
  }

  /**
   * Returns: the next value of the generator
   */
  result_type operator()() {
    result_type z = _mlcg1() - _mlcg2();
    if(z < 1)
      z += MLCG1::modulus-1;
    return z;
  }

  static bool validation(result_type x) { return val == x; }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  /**
   * Writes the state of an \additive_combine generator to a @c
   * std::ostream.  The textual representation of an \additive_combine
   * generator is the textual representation of the first base
   * generator followed by the textual representation of the
   * second base generator.
   */
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const additive_combine& r)
  { os << r._mlcg1 << " " << r._mlcg2; return os; }

  /**
   * Reads the state of an \additive_combine generator from a
   * @c std::istream.
   */
  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, additive_combine& r)
  { is >> r._mlcg1 >> std::ws >> r._mlcg2; return is; }
#endif

  /**
   * Returns: true iff the two \additive_combine generators will
   * produce the same sequence of values.
   */
  friend bool operator==(const additive_combine& x, const additive_combine& y)
  { return x._mlcg1 == y._mlcg1 && x._mlcg2 == y._mlcg2; }
  /**
   * Returns: true iff the two \additive_combine generators will
   * produce different sequences of values.
   */
  friend bool operator!=(const additive_combine& x, const additive_combine& y)
  { return !(x == y); }
#else
  // Use a member function; Streamable concept not supported.
  bool operator==(const additive_combine& rhs) const
  { return _mlcg1 == rhs._mlcg1 && _mlcg2 == rhs._mlcg2; }
  bool operator!=(const additive_combine& rhs) const
  { return !(*this == rhs); }
#endif

private:
  MLCG1 _mlcg1;
  MLCG2 _mlcg2;
};

} // namespace random

/**
 * The specialization \ecuyer1988 was suggested in
 *
 *  @blockquote
 *  "Efficient and Portable Combined Random Number Generators", Pierre L'Ecuyer,
 *  Communications of the ACM, Vol. 31, No. 6, June 1988, pp. 742-749, 774
 *  @endblockquote
 */
typedef random::additive_combine<
    random::linear_congruential<int32_t, 40014, 0, 2147483563, 0>,
    random::linear_congruential<int32_t, 40692, 0, 2147483399, 0>,
  2060321752> ecuyer1988;

} // namespace boost

#endif // BOOST_RANDOM_ADDITIVE_COMBINE_HPP
