/* boost nondet_random.hpp header file
 *
 * Copyright Jens Maurer 2000
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: nondet_random.hpp 62347 2010-05-31 16:44:36Z steven_watanabe $
 *
 * Revision history
 *  2000-02-18  Portability fixes (thanks to Beman Dawes)
 */

//  See http://www.boost.org/libs/random for documentation.


#ifndef BOOST_NONDET_RANDOM_HPP
#define BOOST_NONDET_RANDOM_HPP

#include <string>                       // std::abs
#include <algorithm>                    // std::min
#include <boost/config/no_tr1/cmath.hpp>
#include <boost/config.hpp>
#include <boost/utility.hpp>            // noncopyable
#include <boost/integer_traits.hpp>     // compile-time integral limits
#include <boost/random/detail/auto_link.hpp>

namespace boost {

/**
 * Class \random_device models a \nondeterministic_random_number_generator.
 * It uses one or more implementation-defined stochastic processes to
 * generate a sequence of uniformly distributed non-deterministic random
 * numbers. For those environments where a non-deterministic random number
 * generator is not available, class random_device must not be implemented. See
 *
 *  @blockquote
 *  "Randomness Recommendations for Security", D. Eastlake, S. Crocker,
 *  J. Schiller, Network Working Group, RFC 1750, December 1994
 *  @endblockquote
 *
 * for further discussions. 
 *
 * @xmlnote
 * Some operating systems abstract the computer hardware enough
 * to make it difficult to non-intrusively monitor stochastic processes.
 * However, several do provide a special device for exactly this purpose.
 * It seems to be impossible to emulate the functionality using Standard
 * C++ only, so users should be aware that this class may not be available
 * on all platforms.
 * @endxmlnote
 *
 * <b>Implementation Note for Linux</b>
 *
 * On the Linux operating system, token is interpreted as a filesystem
 * path. It is assumed that this path denotes an operating system
 * pseudo-device which generates a stream of non-deterministic random
 * numbers. The pseudo-device should never signal an error or end-of-file.
 * Otherwise, @c std::ios_base::failure is thrown. By default,
 * \random_device uses the /dev/urandom pseudo-device to retrieve
 * the random numbers. Another option would be to specify the /dev/random
 * pseudo-device, which blocks on reads if the entropy pool has no more
 * random bits available.
 *
 * <b>Implementation Note for Windows</b>
 *
 * On the Windows operating system, token is interpreted as the name
 * of a cryptographic service provider.  By default \random_device uses
 * MS_DEF_PROV.
 *
 * <b>Performance</b>
 *
 * The test program <a href="\boost/libs/random/performance/nondet_random_speed.cpp">
 * nondet_random_speed.cpp</a> measures the execution times of the
 * nondet_random.hpp implementation of the above algorithms in a tight
 * loop. The performance has been evaluated on a Pentium Pro 200 MHz
 * with gcc 2.95.2, Linux 2.2.13, glibc 2.1.2.
 *
 * <table cols="2">
 *   <tr><th>class</th><th>time per invocation [usec]</th></tr>
 *   <tr><td> @xmlonly <classname alt="boost::random_device">random_device</classname> @endxmlonly </td><td>92.0</td></tr>
 * </table>
 *
 * The measurement error is estimated at +/- 1 usec.
 */
class random_device : private noncopyable
{
public:
  typedef unsigned int result_type;
  BOOST_STATIC_CONSTANT(bool, has_fixed_range = true);
  BOOST_STATIC_CONSTANT(result_type, min_value = integer_traits<result_type>::const_min);
  BOOST_STATIC_CONSTANT(result_type, max_value = integer_traits<result_type>::const_max);

  /**
   * Returns: The smallest value that the \random_device can produce.
   */
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return min_value; }
  /**
   * Returns: The largest value that the \random_device can produce.
   */
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return max_value; }
  /** 
   * Constructs a @c random_device, optionally using the given token as an
   * access specification (for example, a URL) to some implementation-defined
   * service for monitoring a stochastic process. 
   */
  BOOST_RANDOM_DECL explicit random_device(const std::string& token = default_token);
  BOOST_RANDOM_DECL ~random_device();
  /**
   * Returns: An entropy estimate for the random numbers returned by
   * operator(), in the range min() to log2( max()+1). A deterministic
   * random number generator (e.g. a pseudo-random number engine)
   * has entropy 0.
   *
   * Throws: Nothing.
   */
  BOOST_RANDOM_DECL double entropy() const;
  /**
   * Returns: A random value in the range [min, max]
   */
  BOOST_RANDOM_DECL unsigned int operator()();

private:
  BOOST_RANDOM_DECL static const char * const default_token;

  /*
   * std:5.3.5/5 [expr.delete]: "If the object being deleted has incomplete
   * class type at the point of deletion and the complete class has a
   * non-trivial destructor [...], the behavior is undefined".
   * This disallows the use of scoped_ptr<> with pimpl-like classes
   * having a non-trivial destructor.
   */
  class impl;
  impl * pimpl;
};


// TODO: put Schneier's Yarrow-160 algorithm here.

} // namespace boost

#endif /* BOOST_NONDET_RANDOM_HPP */
