#pragma once
#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/cmath.hpp"
#include "std/functional.hpp"
#include "std/limits.hpp"
#include "std/type_traits.hpp"

#include <boost/integer.hpp>


namespace my
{

template <typename T> inline T Abs(T x)
{
  return (x < 0 ? -x : x);
}

// Compare floats or doubles for almost equality.
// maxULPs - number of closest floating point values that are considered equal.
// Infinity is treated as almost equal to the largest possible floating point values.
// NaN produces undefined result.
// See https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
// for details.
template <typename TFloat>
bool AlmostEqualULPs(TFloat x, TFloat y, unsigned int maxULPs = 256)
{
  static_assert(is_floating_point<TFloat>::value, "");
  static_assert(numeric_limits<TFloat>::is_iec559, "");

  // Make sure maxUlps is non-negative and small enough that the
  // default NaN won't compare as equal to anything.
  ASSERT_LESS(maxULPs, 4 * 1024 * 1024, ());

  int const bits = CHAR_BIT * sizeof(TFloat);
  typedef typename boost::int_t<bits>::exact IntType;
  typedef typename boost::uint_t<bits>::exact UIntType;

  IntType xInt = *reinterpret_cast<IntType const *>(&x);
  IntType yInt = *reinterpret_cast<IntType const *>(&y);

  // Make xInt and yInt lexicographically ordered as a twos-complement int
  IntType const highestBit = IntType(1) << (bits - 1);
  if (xInt < 0)
    xInt = highestBit - xInt;
  if (yInt < 0)
    yInt = highestBit - yInt;

  UIntType const diff = Abs(xInt - yInt);

  return diff <= maxULPs;
}

// Returns true if x and y are equal up to the absolute difference eps.
// Does not produce a sensible result if any of the arguments is NaN or infinity.
// The default value for eps is deliberately not provided: the intended usage
// is for the client to choose the precision according to the problem domain,
// explicitly define the precision constant and call this function.
template <typename TFloat>
inline bool AlmostEqualAbs(TFloat x, TFloat y, TFloat eps)
{
  return fabs(x - y) < eps;
}

// Returns true if x and y are equal up to the relative difference eps.
// Does not produce a sensible result if any of the arguments is NaN, infinity or zero.
// The same considerations as in AlmostEqualAbs apply.
template <typename TFloat>
inline bool AlmostEqualRel(TFloat x, TFloat y, TFloat eps)
{
  return fabs(x - y) < eps * max(fabs(x), fabs(y));
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

template <typename T, typename TMin, typename TMax>
inline T clamp(T x, TMin xmin, TMax xmax)
{
  if (x > xmax)
    return xmax;
  if (x < xmin)
    return xmin;
  return x;
}

template <typename T>
inline T cyclicClamp(T x, T xmin, T xmax)
{
  if (x > xmax)
    return xmin;
  if (x < xmin)
    return xmax;
  return x;
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

template <typename T>
bool IsIntersect(T const & x0, T const & x1, T const & x2, T const & x3)
{
  return !((x1 < x2) || (x3 < x0));
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

template <typename T> inline T NextModN(T x, T n)
{
  return x + 1 == n ? 0 : x + 1;
}

template <typename T> inline T PrevModN(T x, T n)
{
  return x == 0 ? n - 1 : x - 1;
}

inline uint32_t NextPowOf2(uint32_t v)
{
  v = v - 1;
  v |= (v >> 1);
  v |= (v >> 2);
  v |= (v >> 4);
  v |= (v >> 8);
  v |= (v >> 16);

  return v + 1;
}

// Greatest Common Divisor
template <typename T> T GCD(T a, T b)
{
  T multiplier = 1;
  T gcd = 1;
  while (true)
  {
    if (a == 0 || b == 0)
    {
      gcd = max(a, b);
      break;
    }

    if (a == 1 || b == 1)
    {
      gcd = 1;
      break;
    }

    if ((a & 0x1) == 0 && (b & 0x1) == 0)
    {
      multiplier <<= 1;
      a >>= 1;
      b >>= 1;
      continue;
    }

    if ((a & 0x1) != 0 && (b & 0x1) != 0)
    {
      T const minV = min(a, b);
      T const maxV = max(a, b);
      a = (maxV - minV) >> 1;
      b = minV;
      continue;
    }

    if ((a & 0x1) != 0)
      swap(a, b);

    a >>= 1;
  }

  return multiplier * gcd;
}

/// Calculate hash for the pair of values.
template <typename T1, typename T2>
size_t Hash(T1 const & t1, T2 const & t2)
{
  /// @todo Probably, we need better hash for 2 integral types.
  return (hash<T1>()(t1) ^ (hash<T2>()(t2) << 1));
}

}
