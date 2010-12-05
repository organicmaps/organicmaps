/* boost random/binomial_distribution.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: binomial_distribution.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 */

#ifndef BOOST_RANDOM_BINOMIAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_BINOMIAL_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <cassert>
#include <boost/random/detail/config.hpp>
#include <boost/random/bernoulli_distribution.hpp>

namespace boost {

/**
 * The binomial distribution is an integer valued distribution with
 * two parameters, @c t and @c p.  The values of the distribution
 * are within the range [0,t].
 *
 * The probability that the distribution produces a value k is
 * \f${t \choose k}p^k(1-p)^{t-k}\f$.
 */
template<class IntType = int, class RealType = double>
class binomial_distribution
{
public:
  typedef typename bernoulli_distribution<RealType>::input_type input_type;
  typedef IntType result_type;

  /**
   * Construct an @c binomial_distribution object. @c t and @c p
   * are the parameters of the distribution.
   *
   * Requires: t >=0 && 0 <= p <= 1
   */
  explicit binomial_distribution(IntType t = 1,
                                 const RealType& p = RealType(0.5))
    : _bernoulli(p), _t(t)
  {
    assert(_t >= 0);
    assert(RealType(0) <= p && p <= RealType(1));
  }

  // compiler-generated copy ctor and assignment operator are fine

  /** Returns: the @c t parameter of the distribution */
  IntType t() const { return _t; }
  /** Returns: the @c p parameter of the distribution */
  RealType p() const { return _bernoulli.p(); }
  /**
   * Effects: Subsequent uses of the distribution do not depend
   * on values produced by any engine prior to invoking reset.
   */
  void reset() { }

  /**
   * Returns: a random variate distributed according to the
   * binomial distribution.
   */
  template<class Engine>
  result_type operator()(Engine& eng)
  {
    // TODO: This is O(_t), but it should be O(log(_t)) for large _t
    result_type n = 0;
    for(IntType i = 0; i < _t; ++i)
      if(_bernoulli(eng))
        ++n;
    return n;
  }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  /**
   * Writes the parameters of the distribution to a @c std::ostream.
   */
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const binomial_distribution& bd)
  {
    os << bd._bernoulli << " " << bd._t;
    return os;
  }

  /**
   * Reads the parameters of the distribution from a @c std::istream.
   */
  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, binomial_distribution& bd)
  {
    is >> std::ws >> bd._bernoulli >> std::ws >> bd._t;
    return is;
  }
#endif

private:
  bernoulli_distribution<RealType> _bernoulli;
  IntType _t;
};

} // namespace boost

#endif // BOOST_RANDOM_BINOMIAL_DISTRIBUTION_HPP
