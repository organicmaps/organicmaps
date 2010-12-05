/* boost random/uniform_real.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: uniform_real.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-04-08  added min<max assertion (N. Becker)
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_REAL_HPP
#define BOOST_RANDOM_UNIFORM_REAL_HPP

#include <cassert>
#include <iostream>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/detail/config.hpp>

namespace boost {

/**
 * The distribution function uniform_real models a random distribution.
 * On each invocation, it returns a random floating-point value uniformly
 * distributed in the range [min..max). The value is computed using
 * std::numeric_limits<RealType>::digits random binary digits, i.e.
 * the mantissa of the floating-point value is completely filled with
 * random bits.
 *
 * Note: The current implementation is buggy, because it may not fill
 * all of the mantissa with random bits.
 */
template<class RealType = double>
class uniform_real
{
public:
  typedef RealType input_type;
  typedef RealType result_type;

  /**
   * Constructs a uniform_real object. @c min and @c max are the
   * parameters of the distribution.
   *
   * Requires: min <= max
   */
  explicit uniform_real(RealType min_arg = RealType(0),
                        RealType max_arg = RealType(1))
    : _min(min_arg), _max(max_arg)
  {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    BOOST_STATIC_ASSERT(!std::numeric_limits<RealType>::is_integer);
#endif
    assert(min_arg <= max_arg);
  }

  // compiler-generated copy ctor and assignment operator are fine

  /**
   * Returns: The "min" parameter of the distribution
   */
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _min; }
  /**
   * Returns: The "max" parameter of the distribution
   */
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _max; }
  void reset() { }

  template<class Engine>
  result_type operator()(Engine& eng) {
    result_type numerator = static_cast<result_type>(eng() - eng.min BOOST_PREVENT_MACRO_SUBSTITUTION());
    result_type divisor = static_cast<result_type>(eng.max BOOST_PREVENT_MACRO_SUBSTITUTION() - eng.min BOOST_PREVENT_MACRO_SUBSTITUTION());
    assert(divisor > 0);
    assert(numerator >= 0 && numerator <= divisor);
    return numerator / divisor * (_max - _min) + _min;
  }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const uniform_real& ud)
  {
    os << ud._min << " " << ud._max;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, uniform_real& ud)
  {
    is >> std::ws >> ud._min >> std::ws >> ud._max;
    return is;
  }
#endif

private:
  RealType _min, _max;
};

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_REAL_HPP
