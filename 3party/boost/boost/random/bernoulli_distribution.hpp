/* boost random/bernoulli_distribution.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: bernoulli_distribution.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_BERNOULLI_DISTRIBUTION_HPP
#define BOOST_RANDOM_BERNOULLI_DISTRIBUTION_HPP

#include <cassert>
#include <iostream>
#include <boost/random/detail/config.hpp>

namespace boost {

/**
 * Instantiations of class template \bernoulli_distribution model a
 * \random_distribution. Such a random distribution produces bool values
 * distributed with probabilities P(true) = p and P(false) = 1-p. p is
 * the parameter of the distribution.
 */
template<class RealType = double>
class bernoulli_distribution
{
public:
  // In principle, this could work with both integer and floating-point
  // types.  Generating floating-point random numbers in the first
  // place is probably more expensive, so use integer as input.
  typedef int input_type;
  typedef bool result_type;

  /** 
   * Constructs a \bernoulli_distribution object.
   * p is the parameter of the distribution.
   *
   * Requires: 0 <= p <= 1
   */
  explicit bernoulli_distribution(const RealType& p_arg = RealType(0.5)) 
    : _p(p_arg)
  {
    assert(_p >= 0);
    assert(_p <= 1);
  }

  // compiler-generated copy ctor and assignment operator are fine

  /**
   * Returns: The "p" parameter of the distribution.
   */
  RealType p() const { return _p; }
  /**
   * Effects: Subsequent uses of the distribution do not depend
   * on values produced by any engine prior to invoking reset.
   */
  void reset() { }

  /**
   * Returns: a random variate distributed according to the
   * \bernoulli_distribution.
   */
  template<class Engine>
  result_type operator()(Engine& eng)
  {
    if(_p == RealType(0))
      return false;
    else
      return RealType(eng() - (eng.min)()) <= _p * RealType((eng.max)()-(eng.min)());
  }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  /**
   * Writes the parameters of the distribution to a @c std::ostream.
   */
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const bernoulli_distribution& bd)
  {
    os << bd._p;
    return os;
  }

  /**
   * Reads the parameters of the distribution from a @c std::istream.
   */
  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, bernoulli_distribution& bd)
  {
    is >> std::ws >> bd._p;
    return is;
  }
#endif

private:
  RealType _p;
};

} // namespace boost

#endif // BOOST_RANDOM_BERNOULLI_DISTRIBUTION_HPP
