/* boost random/uniform_int.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: uniform_int.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-04-08  added min<max assertion (N. Becker)
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_INT_HPP
#define BOOST_RANDOM_UNIFORM_INT_HPP

#include <cassert>
#include <iostream>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/signed_unsigned_tools.hpp>
#include <boost/type_traits/make_unsigned.hpp>

namespace boost {

/**
 * The distribution function uniform_int models a \random_distribution.
 * On each invocation, it returns a random integer value uniformly
 * distributed in the set of integer numbers {min, min+1, min+2, ..., max}.
 *
 * The template parameter IntType shall denote an integer-like value type.
 */
template<class IntType = int>
class uniform_int
{
public:
  typedef IntType input_type;
  typedef IntType result_type;

  /// \cond hide_private_members
  typedef typename make_unsigned<result_type>::type range_type;
  /// \endcond

  /**
   * Constructs a uniform_int object. @c min and @c max are
   * the parameters of the distribution.
   *
   * Requires: min <= max
   */
  explicit uniform_int(IntType min_arg = 0, IntType max_arg = 9)
    : _min(min_arg), _max(max_arg)
  {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    // MSVC fails BOOST_STATIC_ASSERT with std::numeric_limits at class scope
    BOOST_STATIC_ASSERT(std::numeric_limits<IntType>::is_integer);
#endif
    assert(min_arg <= max_arg);
    init();
  }

  /**
   * Returns: The "min" parameter of the distribution
   */
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _min; }
  /**
   * Returns: The "max" parameter of the distribution
   */
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _max; }
  void reset() { }
  
  // can't have member function templates out-of-line due to MSVC bugs
  template<class Engine>
  result_type operator()(Engine& eng)
  {
      return generate(eng, _min, _max, _range);
  }

  template<class Engine>
  result_type operator()(Engine& eng, result_type n)
  {
      assert(n > 0);

      if (n == 1)
      {
        return 0;
      }

      return generate(eng, 0, n - 1, n - 1);
  }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const uniform_int& ud)
  {
    os << ud._min << " " << ud._max;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, uniform_int& ud)
  {
    is >> std::ws >> ud._min >> std::ws >> ud._max;
    ud.init();
    return is;
  }
#endif

private:

#ifdef BOOST_MSVC
#pragma warning(push)
// disable division by zero warning, since we can't
// actually divide by zero.
#pragma warning(disable:4723)
#endif

  /// \cond hide_private_members
  template<class Engine>
  static result_type generate(Engine& eng, result_type min_value, result_type /*max_value*/, range_type range)
  {
    typedef typename Engine::result_type base_result;
    // ranges are always unsigned
    typedef typename make_unsigned<base_result>::type base_unsigned;
    const base_result bmin = (eng.min)();
    const base_unsigned brange =
      random::detail::subtract<base_result>()((eng.max)(), (eng.min)());

    if(range == 0) {
      return min_value;    
    } else if(brange == range) {
      // this will probably never happen in real life
      // basically nothing to do; just take care we don't overflow / underflow
      base_unsigned v = random::detail::subtract<base_result>()(eng(), bmin);
      return random::detail::add<base_unsigned, result_type>()(v, min_value);
    } else if(brange < range) {
      // use rejection method to handle things like 0..3 --> 0..4
      for(;;) {
        // concatenate several invocations of the base RNG
        // take extra care to avoid overflows

        //  limit == floor((range+1)/(brange+1))
        //  Therefore limit*(brange+1) <= range+1
        range_type limit;
        if(range == (std::numeric_limits<range_type>::max)()) {
          limit = range/(range_type(brange)+1);
          if(range % (range_type(brange)+1) == range_type(brange))
            ++limit;
        } else {
          limit = (range+1)/(range_type(brange)+1);
        }

        // We consider "result" as expressed to base (brange+1):
        // For every power of (brange+1), we determine a random factor
        range_type result = range_type(0);
        range_type mult = range_type(1);

        // loop invariants:
        //  result < mult
        //  mult <= range
        while(mult <= limit) {
          // Postcondition: result <= range, thus no overflow
          //
          // limit*(brange+1)<=range+1                   def. of limit       (1)
          // eng()-bmin<=brange                          eng() post.         (2)
          // and mult<=limit.                            loop condition      (3)
          // Therefore mult*(eng()-bmin+1)<=range+1      by (1),(2),(3)      (4)
          // Therefore mult*(eng()-bmin)+mult<=range+1   rearranging (4)     (5)
          // result<mult                                 loop invariant      (6)
          // Therefore result+mult*(eng()-bmin)<range+1  by (5), (6)         (7)
          //
          // Postcondition: result < mult*(brange+1)
          //
          // result<mult                                 loop invariant      (1)
          // eng()-bmin<=brange                          eng() post.         (2)
          // Therefore result+mult*(eng()-bmin) <
          //           mult+mult*(eng()-bmin)            by (1)              (3)
          // Therefore result+(eng()-bmin)*mult <
          //           mult+mult*brange                  by (2), (3)         (4)
          // Therefore result+(eng()-bmin)*mult <
          //           mult*(brange+1)                   by (4)
          result += static_cast<range_type>(random::detail::subtract<base_result>()(eng(), bmin) * mult);

          // equivalent to (mult * (brange+1)) == range+1, but avoids overflow.
          if(mult * range_type(brange) == range - mult + 1) {
              // The destination range is an integer power of
              // the generator's range.
              return(result);
          }

          // Postcondition: mult <= range
          // 
          // limit*(brange+1)<=range+1                   def. of limit       (1)
          // mult<=limit                                 loop condition      (2)
          // Therefore mult*(brange+1)<=range+1          by (1), (2)         (3)
          // mult*(brange+1)!=range+1                    preceding if        (4)
          // Therefore mult*(brange+1)<range+1           by (3), (4)         (5)
          // 
          // Postcondition: result < mult
          //
          // See the second postcondition on the change to result. 
          mult *= range_type(brange)+range_type(1);
        }
        // loop postcondition: range/mult < brange+1
        //
        // mult > limit                                  loop condition      (1)
        // Suppose range/mult >= brange+1                Assumption          (2)
        // range >= mult*(brange+1)                      by (2)              (3)
        // range+1 > mult*(brange+1)                     by (3)              (4)
        // range+1 > (limit+1)*(brange+1)                by (1), (4)         (5)
        // (range+1)/(brange+1) > limit+1                by (5)              (6)
        // limit < floor((range+1)/(brange+1))           by (6)              (7)
        // limit==floor((range+1)/(brange+1))            def. of limit       (8)
        // not (2)                                       reductio            (9)
        //
        // loop postcondition: (range/mult)*mult+(mult-1) >= range
        //
        // (range/mult)*mult + range%mult == range       identity            (1)
        // range%mult < mult                             def. of %           (2)
        // (range/mult)*mult+mult > range                by (1), (2)         (3)
        // (range/mult)*mult+(mult-1) >= range           by (3)              (4)
        //
        // Note that the maximum value of result at this point is (mult-1),
        // so after this final step, we generate numbers that can be
        // at least as large as range.  We have to really careful to avoid
        // overflow in this final addition and in the rejection.  Anything
        // that overflows is larger than range and can thus be rejected.

        // range/mult < brange+1  -> no endless loop
        range_type result_increment = uniform_int<range_type>(0, range/mult)(eng);
        if((std::numeric_limits<range_type>::max)() / mult < result_increment) {
          // The multiplcation would overflow.  Reject immediately.
          continue;
        }
        result_increment *= mult;
        // unsigned integers are guaranteed to wrap on overflow.
        result += result_increment;
        if(result < result_increment) {
          // The addition overflowed.  Reject.
          continue;
        }
        if(result > range) {
          // Too big.  Reject.
          continue;
        }
        return random::detail::add<range_type, result_type>()(result, min_value);
      }
    } else {                   // brange > range
      base_unsigned bucket_size;
      // it's safe to add 1 to range, as long as we cast it first,
      // because we know that it is less than brange.  However,
      // we do need to be careful not to cause overflow by adding 1
      // to brange.
      if(brange == (std::numeric_limits<base_unsigned>::max)()) {
        bucket_size = brange / (static_cast<base_unsigned>(range)+1);
        if(brange % (static_cast<base_unsigned>(range)+1) == static_cast<base_unsigned>(range)) {
          ++bucket_size;
        }
      } else {
        bucket_size = (brange+1) / (static_cast<base_unsigned>(range)+1);
      }
      for(;;) {
        base_unsigned result =
          random::detail::subtract<base_result>()(eng(), bmin);
        result /= bucket_size;
        // result and range are non-negative, and result is possibly larger
        // than range, so the cast is safe
        if(result <= static_cast<base_unsigned>(range))
          return random::detail::add<base_unsigned, result_type>()(result, min_value);
      }
    }
  }

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

  void init()
  {
    _range = random::detail::subtract<result_type>()(_max, _min);
  }

  /// \endcond

  // The result_type may be signed or unsigned, but the _range is always
  // unsigned.
  result_type _min, _max;
  range_type _range;
};

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_INT_HPP
