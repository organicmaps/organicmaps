/* boost random/uniform_on_sphere.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: uniform_on_sphere.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_ON_SPHERE_HPP
#define BOOST_RANDOM_UNIFORM_ON_SPHERE_HPP

#include <vector>
#include <algorithm>     // std::transform
#include <functional>    // std::bind2nd, std::divides
#include <boost/random/detail/config.hpp>
#include <boost/random/normal_distribution.hpp>

namespace boost {

/**
 * Instantiations of class template uniform_on_sphere model a
 * \random_distribution. Such a distribution produces random
 * numbers uniformly distributed on the unit sphere of arbitrary
 * dimension @c dim. The @c Cont template parameter must be a STL-like
 * container type with begin and end operations returning non-const
 * ForwardIterators of type @c Cont::iterator. Each invocation of the
 * @c UniformRandomNumberGenerator shall result in a floating-point
 * value in the range [0,1). 
 */
template<class RealType = double, class Cont = std::vector<RealType> >
class uniform_on_sphere
{
public:
  typedef RealType input_type;
  typedef Cont result_type;

  /**
   * Constructs a @c uniform_on_sphere distribution.
   * @c dim is the dimension of the sphere.
   */
  explicit uniform_on_sphere(int dim = 2) : _container(dim), _dim(dim) { }

  // compiler-generated copy ctor and assignment operator are fine

  void reset() { _normal.reset(); }

  template<class Engine>
  const result_type & operator()(Engine& eng)
  {
    RealType sqsum = 0;
    for(typename Cont::iterator it = _container.begin();
        it != _container.end();
        ++it) {
      RealType val = _normal(eng);
      *it = val;
      sqsum += val * val;
    }
#ifndef BOOST_NO_STDC_NAMESPACE
    using std::sqrt;
#endif
    // for all i: result[i] /= sqrt(sqsum)
    std::transform(_container.begin(), _container.end(), _container.begin(),
                   std::bind2nd(std::divides<RealType>(), sqrt(sqsum)));
    return _container;
  }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const uniform_on_sphere& sd)
  {
    os << sd._dim;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, uniform_on_sphere& sd)
  {
    is >> std::ws >> sd._dim;
    sd._container.resize(sd._dim);
    return is;
  }
#endif

private:
  normal_distribution<RealType> _normal;
  result_type _container;
  int _dim;
};

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_ON_SPHERE_HPP
