/* boost random/triangle_distribution.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: triangle_distribution.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP
#define BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <cassert>
#include <boost/random/detail/config.hpp>
#include <boost/random/uniform_01.hpp>

namespace boost {

/**
 * Instantiations of @c triangle_distribution model a \random_distribution.
 * A @c triangle_distribution has three parameters, @c a, @c b, and @c c,
 * which are the smallest, the most probable and the largest values of
 * the distribution respectively.
 */
template<class RealType = double>
class triangle_distribution
{
public:
  typedef RealType input_type;
  typedef RealType result_type;

  /**
   * Constructs a @c triangle_distribution with the parameters
   * @c a, @c b, and @c c.
   *
   * Preconditions: a <= b <= c.
   */
  explicit triangle_distribution(result_type a_arg = result_type(0),
                                 result_type b_arg = result_type(0.5),
                                 result_type c_arg = result_type(1))
    : _a(a_arg), _b(b_arg), _c(c_arg)
  {
    assert(_a <= _b && _b <= _c);
    init();
  }

  // compiler-generated copy ctor and assignment operator are fine

  /** Returns the @c a parameter of the distribution */
  result_type a() const { return _a; }
  /** Returns the @c b parameter of the distribution */
  result_type b() const { return _b; }
  /** Returns the @c c parameter of the distribution */
  result_type c() const { return _c; }

  void reset() { }

  template<class Engine>
  result_type operator()(Engine& eng)
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    using std::sqrt;
#endif
    result_type u = eng();
    if( u <= q1 )
      return _a + p1*sqrt(u);
    else
      return _c - d3*sqrt(d2*u-d1);
  }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const triangle_distribution& td)
  {
    os << td._a << " " << td._b << " " << td._c;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, triangle_distribution& td)
  {
    is >> std::ws >> td._a >> std::ws >> td._b >> std::ws >> td._c;
    td.init();
    return is;
  }
#endif

private:
  /// \cond hide_private_members
  void init()
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    using std::sqrt;
#endif
    d1 = _b - _a;
    d2 = _c - _a;
    d3 = sqrt(_c - _b);
    q1 = d1 / d2;
    p1 = sqrt(d1 * d2);
  }
  /// \endcond

  result_type _a, _b, _c;
  result_type d1, d2, d3, q1, p1;
};

} // namespace boost

#endif // BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP
