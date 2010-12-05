/* boost random/mersenne_twister.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: mersenne_twister.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_MERSENNE_TWISTER_HPP
#define BOOST_RANDOM_MERSENNE_TWISTER_HPP

#include <iostream>
#include <algorithm>     // std::copy
#include <stdexcept>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/cstdint.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/ptr_helper.hpp>
#include <boost/random/detail/seed.hpp>

namespace boost {
namespace random {

/**
 * Instantiations of class template mersenne_twister model a
 * \pseudo_random_number_generator. It uses the algorithm described in
 *
 *  @blockquote
 *  "Mersenne Twister: A 623-dimensionally equidistributed uniform
 *  pseudo-random number generator", Makoto Matsumoto and Takuji Nishimura,
 *  ACM Transactions on Modeling and Computer Simulation: Special Issue on
 *  Uniform Random Number Generation, Vol. 8, No. 1, January 1998, pp. 3-30. 
 *  @endblockquote
 *
 * @xmlnote
 * The boost variant has been implemented from scratch and does not
 * derive from or use mt19937.c provided on the above WWW site. However, it
 * was verified that both produce identical output.
 * @endxmlnote
 *
 * The seeding from an integer was changed in April 2005 to address a
 * <a href="http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/emt19937ar.html">weakness</a>.
 * 
 * The quality of the generator crucially depends on the choice of the
 * parameters.  User code should employ one of the sensibly parameterized
 * generators such as \mt19937 instead.
 *
 * The generator requires considerable amounts of memory for the storage of
 * its state array. For example, \mt11213b requires about 1408 bytes and
 * \mt19937 requires about 2496 bytes.
 */
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
class mersenne_twister
{
public:
  typedef UIntType result_type;
  BOOST_STATIC_CONSTANT(int, word_size = w);
  BOOST_STATIC_CONSTANT(int, state_size = n);
  BOOST_STATIC_CONSTANT(int, shift_size = m);
  BOOST_STATIC_CONSTANT(int, mask_bits = r);
  BOOST_STATIC_CONSTANT(UIntType, parameter_a = a);
  BOOST_STATIC_CONSTANT(int, output_u = u);
  BOOST_STATIC_CONSTANT(int, output_s = s);
  BOOST_STATIC_CONSTANT(UIntType, output_b = b);
  BOOST_STATIC_CONSTANT(int, output_t = t);
  BOOST_STATIC_CONSTANT(UIntType, output_c = c);
  BOOST_STATIC_CONSTANT(int, output_l = l);

  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
  
  /**
   * Constructs a @c mersenne_twister and calls @c seed().
   */
  mersenne_twister() { seed(); }

  /**
   * Constructs a @c mersenne_twister and calls @c seed(value).
   */
  BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(mersenne_twister, UIntType, value)
  { seed(value); }
  template<class It> mersenne_twister(It& first, It last) { seed(first,last); }

  /**
   * Constructs a mersenne_twister and calls @c seed(gen).
   *
   * @xmlnote
   * The copy constructor will always be preferred over
   * the templated constructor.
   * @endxmlnote
   */
  BOOST_RANDOM_DETAIL_GENERATOR_CONSTRUCTOR(mersenne_twister, Generator, gen)
  { seed(gen); }

  // compiler-generated copy ctor and assignment operator are fine

  /** Calls @c seed(result_type(5489)). */
  void seed() { seed(UIntType(5489)); }

  /**
   * Sets the state x(0) to v mod 2w. Then, iteratively,
   * sets x(i) to (i + 1812433253 * (x(i-1) xor (x(i-1) rshift w-2))) mod 2<sup>w</sup>
   * for i = 1 .. n-1. x(n) is the first value to be returned by operator().
   */
  BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(mersenne_twister, UIntType, value)
  {
    // New seeding algorithm from 
    // http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/emt19937ar.html
    // In the previous versions, MSBs of the seed affected only MSBs of the
    // state x[].
    const UIntType mask = ~0u;
    x[0] = value & mask;
    for (i = 1; i < n; i++) {
      // See Knuth "The Art of Computer Programming" Vol. 2, 3rd ed., page 106
      x[i] = (1812433253UL * (x[i-1] ^ (x[i-1] >> (w-2))) + i) & mask;
    }
  }

  /**
   * Sets the state of this mersenne_twister to the values
   * returned by n invocations of gen.
   *
   * Complexity: Exactly n invocations of gen.
   */
  BOOST_RANDOM_DETAIL_GENERATOR_SEED(mersenne_twister, Generator, gen)
  {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    BOOST_STATIC_ASSERT(!std::numeric_limits<result_type>::is_signed);
#endif
    // I could have used std::generate_n, but it takes "gen" by value
    for(int j = 0; j < n; j++)
      x[j] = gen();
    i = n;
  }

  template<class It>
  void seed(It& first, It last)
  {
    int j;
    for(j = 0; j < n && first != last; ++j, ++first)
      x[j] = *first;
    i = n;
    if(first == last && j < n)
      throw std::invalid_argument("mersenne_twister::seed");
  }
  
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return 0; }
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const
  {
    // avoid "left shift count >= with of type" warning
    result_type res = 0;
    for(int j = 0; j < w; ++j)
      res |= (1u << j);
    return res;
  }

  result_type operator()();
  static bool validation(result_type v) { return val == v; }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const mersenne_twister& mt)
  {
    for(int j = 0; j < mt.state_size; ++j)
      os << mt.compute(j) << " ";
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, mersenne_twister& mt)
  {
    for(int j = 0; j < mt.state_size; ++j)
      is >> mt.x[j] >> std::ws;
    // MSVC (up to 7.1) and Borland (up to 5.64) don't handle the template
    // value parameter "n" available from the class template scope, so use
    // the static constant with the same value
    mt.i = mt.state_size;
    return is;
  }
#endif

  friend bool operator==(const mersenne_twister& x, const mersenne_twister& y)
  {
    for(int j = 0; j < state_size; ++j)
      if(x.compute(j) != y.compute(j))
        return false;
    return true;
  }

  friend bool operator!=(const mersenne_twister& x, const mersenne_twister& y)
  { return !(x == y); }
#else
  // Use a member function; Streamable concept not supported.
  bool operator==(const mersenne_twister& rhs) const
  {
    for(int j = 0; j < state_size; ++j)
      if(compute(j) != rhs.compute(j))
        return false;
    return true;
  }

  bool operator!=(const mersenne_twister& rhs) const
  { return !(*this == rhs); }
#endif

private:
  /// \cond hide_private_members
  // returns x(i-n+index), where index is in 0..n-1
  UIntType compute(unsigned int index) const
  {
    // equivalent to (i-n+index) % 2n, but doesn't produce negative numbers
    return x[ (i + n + index) % (2*n) ];
  }
  void twist(int block);
  /// \endcond

  // state representation: next output is o(x(i))
  //   x[0]  ... x[k] x[k+1] ... x[n-1]     x[n]     ... x[2*n-1]   represents
  //  x(i-k) ... x(i) x(i+1) ... x(i-k+n-1) x(i-k-n) ... x[i(i-k-1)]
  // The goal is to always have x(i-n) ... x(i-1) available for
  // operator== and save/restore.

  UIntType x[2*n]; 
  int i;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const bool mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::has_fixed_range;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const int mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::state_size;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const int mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::shift_size;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const int mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::mask_bits;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const UIntType mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::parameter_a;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const int mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::output_u;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const int mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::output_s;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const UIntType mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::output_b;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const int mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::output_t;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const UIntType mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::output_c;
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
const int mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::output_l;
#endif

/// \cond hide_private_members
template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
void mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::twist(int block)
{
  const UIntType upper_mask = (~0u) << r;
  const UIntType lower_mask = ~upper_mask;

  if(block == 0) {
    for(int j = n; j < 2*n; j++) {
      UIntType y = (x[j-n] & upper_mask) | (x[j-(n-1)] & lower_mask);
      x[j] = x[j-(n-m)] ^ (y >> 1) ^ (y&1 ? a : 0);
    }
  } else if (block == 1) {
    // split loop to avoid costly modulo operations
    {  // extra scope for MSVC brokenness w.r.t. for scope
      for(int j = 0; j < n-m; j++) {
        UIntType y = (x[j+n] & upper_mask) | (x[j+n+1] & lower_mask);
        x[j] = x[j+n+m] ^ (y >> 1) ^ (y&1 ? a : 0);
      }
    }
    
    for(int j = n-m; j < n-1; j++) {
      UIntType y = (x[j+n] & upper_mask) | (x[j+n+1] & lower_mask);
      x[j] = x[j-(n-m)] ^ (y >> 1) ^ (y&1 ? a : 0);
    }
    // last iteration
    UIntType y = (x[2*n-1] & upper_mask) | (x[0] & lower_mask);
    x[n-1] = x[m-1] ^ (y >> 1) ^ (y&1 ? a : 0);
    i = 0;
  }
}
/// \endcond

template<class UIntType, int w, int n, int m, int r, UIntType a, int u,
  int s, UIntType b, int t, UIntType c, int l, UIntType val>
inline typename mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::result_type
mersenne_twister<UIntType,w,n,m,r,a,u,s,b,t,c,l,val>::operator()()
{
  if(i == n)
    twist(0);
  else if(i >= 2*n)
    twist(1);
  // Step 4
  UIntType z = x[i];
  ++i;
  z ^= (z >> u);
  z ^= ((z << s) & b);
  z ^= ((z << t) & c);
  z ^= (z >> l);
  return z;
}

} // namespace random

/**
 * The specializations \mt11213b and \mt19937 are from
 *
 *  @blockquote
 *  "Mersenne Twister: A 623-dimensionally equidistributed
 *  uniform pseudo-random number generator", Makoto Matsumoto
 *  and Takuji Nishimura, ACM Transactions on Modeling and
 *  Computer Simulation: Special Issue on Uniform Random Number
 *  Generation, Vol. 8, No. 1, January 1998, pp. 3-30. 
 *  @endblockquote
 */
typedef random::mersenne_twister<uint32_t,32,351,175,19,0xccab8ee7,11,
  7,0x31b6ab00,15,0xffe50000,17, 0xa37d3c92> mt11213b;

/**
 * The specializations \mt11213b and \mt19937 are from
 *
 *  @blockquote
 *  "Mersenne Twister: A 623-dimensionally equidistributed
 *  uniform pseudo-random number generator", Makoto Matsumoto
 *  and Takuji Nishimura, ACM Transactions on Modeling and
 *  Computer Simulation: Special Issue on Uniform Random Number
 *  Generation, Vol. 8, No. 1, January 1998, pp. 3-30. 
 *  @endblockquote
 */
typedef random::mersenne_twister<uint32_t,32,624,397,31,0x9908b0df,11,
  7,0x9d2c5680,15,0xefc60000,18, 3346425566U> mt19937;

} // namespace boost

BOOST_RANDOM_PTR_HELPER_SPEC(boost::mt19937)

#endif // BOOST_RANDOM_MERSENNE_TWISTER_HPP
