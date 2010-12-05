/* boost random/ranlux.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: ranlux.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-02-18  created
 */

#ifndef BOOST_RANDOM_RANLUX_HPP
#define BOOST_RANDOM_RANLUX_HPP

#include <boost/config.hpp>
#include <boost/random/subtract_with_carry.hpp>
#include <boost/random/discard_block.hpp>

namespace boost {

namespace random {
  typedef subtract_with_carry<int, (1<<24), 10, 24, 0> ranlux_base;
  typedef subtract_with_carry_01<float, 24, 10, 24> ranlux_base_01;
  typedef subtract_with_carry_01<double, 48, 10, 24> ranlux64_base_01;
}

namespace random {
namespace detail {
/**
 * The ranlux family of generators are described in
 *
 *  @blockquote
 *  "A portable high-quality random number generator for lattice field theory
 *  calculations", M. Luescher, Computer Physics Communications, 79 (1994)
 *  pp 100-110. 
 *  @endblockquote
 *
 * The levels are given in
 * 
 *  @blockquote
 *  "RANLUX: A Fortran implementation ofthe high-quality
 *  pseudorandom number generator of Luescher", F. James,
 *  Computer Physics Communications 79 (1994) 111-114
 *  @endblockquote
 */
class ranlux_documentation {};
}
}

/** @copydoc boost::random::detail::ranlux_documentation */
typedef random::discard_block<random::ranlux_base, 223, 24> ranlux3;
/** @copydoc boost::random::detail::ranlux_documentation */
typedef random::discard_block<random::ranlux_base, 389, 24> ranlux4;

/** @copydoc boost::random::detail::ranlux_documentation */
typedef random::discard_block<random::ranlux_base_01, 223, 24> ranlux3_01;
/** @copydoc boost::random::detail::ranlux_documentation */
typedef random::discard_block<random::ranlux_base_01, 389, 24> ranlux4_01;

/** @copydoc boost::random::detail::ranlux_documentation */
typedef random::discard_block<random::ranlux64_base_01, 223, 24> ranlux64_3_01;
/** @copydoc boost::random::detail::ranlux_documentation */
typedef random::discard_block<random::ranlux64_base_01, 389, 24> ranlux64_4_01;

#if !defined(BOOST_NO_INT64_T) && !defined(BOOST_NO_INTEGRAL_INT64_T)
namespace random {
  typedef random::subtract_with_carry<int64_t, (int64_t(1)<<48), 10, 24, 0> ranlux64_base;
}
/** @copydoc boost::random::detail::ranlux_documentation */
typedef random::discard_block<random::ranlux64_base, 223, 24> ranlux64_3;
/** @copydoc boost::random::detail::ranlux_documentation */
typedef random::discard_block<random::ranlux64_base, 389, 24> ranlux64_4;
#endif /* !BOOST_NO_INT64_T && !BOOST_NO_INTEGRAL_INT64_T */

} // namespace boost

#endif // BOOST_RANDOM_LINEAR_CONGRUENTIAL_HPP
