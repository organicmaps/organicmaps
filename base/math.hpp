#pragma once
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../std/cmath.hpp"
#include "../std/limits.hpp"
#include "../std/type_traits.hpp"
#include <boost/integer.hpp>

namespace my
{

// Compare floats or doubles for almost equality.
// maxULPs - number of closest floating point values that are considered equal.
// Infinity is treated as almost equal to the largest possible floating point values.
// NaN produces undefined result.
// See http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm for details.
template <typename FloatT> bool AlmostEqual(FloatT x, FloatT y, unsigned int maxULPs = 64)
{
  STATIC_ASSERT(is_floating_point<FloatT>::value);
  STATIC_ASSERT(numeric_limits<FloatT>::is_iec559);
  STATIC_ASSERT(!numeric_limits<FloatT>::is_exact);
  STATIC_ASSERT(!numeric_limits<FloatT>::is_integer);
  // Make sure maxUlps is non-negative and small enough that the
  // default NAN won't compare as equal to anything.
  ASSERT_LESS(maxULPs, 4 * 1024 * 1024, ());

  typedef typename boost::int_t<8 * sizeof(FloatT)>::exact IntType;

  IntType xInt = *reinterpret_cast<IntType const *>(&x);
  IntType yInt = *reinterpret_cast<IntType const *>(&y);

  // Make xInt and yInt lexicographically ordered as a twos-complement int
  IntType const highestBit = IntType(1) << (sizeof(FloatT) * 8 - 1);
  if (xInt < 0)
    xInt = highestBit - xInt;
  if (yInt < 0)
    yInt = highestBit - yInt;

  IntType const diff = Abs(xInt - yInt);

  return diff <= static_cast<IntType>(maxULPs);
}

template <typename TFloat> inline TFloat DegToRad(TFloat deg)
{
  return deg * TFloat(math::pi) / TFloat(180);
}

template <typename TFloat> inline TFloat RadToDeg(TFloat rad)
{
  return rad * TFloat(180) / TFloat(math::pi);
}

template <typename T> inline T id(T const & x)
{
  return x;
}

template <typename T> inline T sq(T const & x)
{
  return x * x;
}

template <typename T, typename TMinMax> inline T clamp(T x, TMinMax xmin, TMinMax xmax)
{
  if (x > xmax)
    return xmax;
  if (x < xmin)
    return xmin;
  return x;
}

template <typename T> inline T Abs(T x)
{
  return x >= 0 ? x : -x;
}

template <typename T> inline bool between_s(T a, T b, T x)
{
  return (a <= x && x <= b);
}
template <typename T> inline bool between_i(T a, T b, T x)
{
  return (a < x && x < b);
}

inline int rounds(double x)
{
  return (x > 0.0 ? int(x + 0.5) : int(x - 0.5));
}

inline size_t SizeAligned(size_t size, size_t align)
{
  // static_cast    .
  return size + (static_cast<size_t>(-static_cast<ptrdiff_t>(size)) & (align - 1));
}

// Computes x^n.
template <typename T> inline T PowUint(T x, uint64_t n)
{
  T res = 1;
  for (T t = x; n > 0; n >>= 1, t *= t)
    if (n & 1)
      res *= t;
  return res;
}

}
