/* boost random/poisson_distribution.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: poisson_distribution.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 */

#ifndef BOOST_RANDOM_POISSON_DISTRIBUTION_HPP
#define BOOST_RANDOM_POISSON_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <cassert>
#include <iostream>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/detail/config.hpp>

namespace boost {

// Knuth

/**
 * An instantiation of the class template @c poisson_distribution is a
 * model of \random_distribution.  The poisson distribution has
 * \f$p(i) = \frac{e^{-\lambda}\lambda^i}{i!}\f$
 */
template<class IntType = int, class RealType = double>
class poisson_distribution
{
public:
  typedef RealType input_type;
  typedef IntType result_type;

  /**
   * Constructs a @c poisson_distribution with the parameter @c mean.
   *
   * Requires: mean > 0
   */
  explicit poisson_distribution(const RealType& mean_arg = RealType(1))
    : _mean(mean_arg)
  {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    // MSVC fails BOOST_STATIC_ASSERT with std::numeric_limits at class scope
    BOOST_STATIC_ASSERT(std::numeric_limits<IntType>::is_integer);
    BOOST_STATIC_ASSERT(!std::numeric_limits<RealType>::is_integer);
#endif

    assert(_mean > RealType(0));
    init();
  }

  // compiler-generated copy ctor and assignment operator are fine

  /**
   * Returns: the "mean" parameter of the distribution.
   */
  RealType mean() const { return _mean; }
  void reset() { }

  template<class Engine>
  result_type operator()(Engine& eng)
  {
    // TODO: This is O(_mean), but it should be O(log(_mean)) for large _mean
    RealType product = RealType(1);
    for(result_type m = 0; ; ++m) {
      product *= eng();
      if(product <= _exp_mean)
        return m;
    }
  }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const poisson_distribution& pd)
  {
    os << pd._mean;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, poisson_distribution& pd)
  {
    is >> std::ws >> pd._mean;
    pd.init();
    return is;
  }
#endif

private:
  /// \cond hide_private_members
  void init()
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    // allow for Koenig lookup
    using std::exp;
#endif
    _exp_mean = exp(-_mean);
  }
  /// \endcond

  RealType _mean;
  // some precomputed data from the parameters
  RealType _exp_mean;
};

} // namespace boost

#endif // BOOST_RANDOM_POISSON_DISTRIBUTION_HPP
