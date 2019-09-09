#pragma once

#include "base/assert.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <functional>
#include <limits>
#include <type_traits>

#include <boost/integer.hpp>

namespace math
{
double constexpr pi = 3.14159265358979323846;
double constexpr pi2 = pi / 2.0;
double constexpr pi4 = pi / 4.0;
}  // namespace math

namespace base
{
template <typename T>
T Abs(T x)
{
  return (x < 0 ? -x : x);
}

template <typename Number,
          typename EnableIf = typename std::enable_if_t<
              std::is_integral<Number>::value || std::is_floating_point<Number>::value, void>>
int constexpr Sign(Number const number) noexcept
{
  return number == 0 ? 0 : number > 0 ? 1 : -1;
}

// Compare floats or doubles for almost equality.
// maxULPs - number of closest floating point values that are considered equal.
// Infinity is treated as almost equal to the largest possible floating point values.
// NaN produces undefined result.
//
// This function is deprecated. Use AlmostEqualAbs, AlmostEqualRel or AlmostEqualAbsOrRel instead.
// See https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
// for details.
template <typename Float>
bool AlmostEqualULPs(Float x, Float y, unsigned int maxULPs = 256)
{
  static_assert(std::is_floating_point<Float>::value, "");
  static_assert(std::numeric_limits<Float>::is_iec559, "");

  // Make sure maxUlps is non-negative and small enough that the
  // default NaN won't compare as equal to anything.
  ASSERT_LESS(maxULPs, 4 * 1024 * 1024, ());

  int const bits = CHAR_BIT * sizeof(Float);
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
template <typename Float>
bool AlmostEqualAbs(Float x, Float y, Float eps)
{
  return fabs(x - y) < eps;
}

// Returns true if x and y are equal up to the relative difference eps.
// Does not produce a sensible result if any of the arguments is NaN, infinity or zero.
// The same considerations as in AlmostEqualAbs apply.
template <typename Float>
bool AlmostEqualRel(Float x, Float y, Float eps)
{
  return fabs(x - y) < eps * std::max(fabs(x), fabs(y));
}

// Returns true if x and y are equal up to the absolute or relative difference eps.
template <typename Float>
bool AlmostEqualAbsOrRel(Float x, Float y, Float eps)
{
  return AlmostEqualAbs(x, y, eps) || AlmostEqualRel(x, y, eps);
}

template <typename Float>
Float constexpr DegToRad(Float deg)
{
  return deg * Float(math::pi) / Float(180);
}

template <typename Float>
Float constexpr RadToDeg(Float rad)
{
  return rad * Float(180) / Float(math::pi);
}

template <typename T>
T Clamp(T const x, T const xmin, T const xmax)
{
  if (x > xmax)
    return xmax;
  if (x < xmin)
    return xmin;
  return x;
}

template <typename T>
bool Between(T const a, T const b, T const x)
{
  return a <= x && x <= b;
}

// This function is deprecated. Use std::round instead.
inline int SignedRound(double x)
{
  return x > 0.0 ? static_cast<int>(x + 0.5) : static_cast<int>(x - 0.5);
}

// Computes x^n.
template <typename T>
T PowUint(T x, uint64_t n)
{
  T res = 1;
  for (T t = x; n > 0; n >>= 1, t *= t)
  {
    if (n & 1)
      res *= t;
  }
  return res;
}

template <typename T>
T NextModN(T x, T n)
{
  ASSERT_GREATER(n, 0, ());
  return x + 1 == n ? 0 : x + 1;
}

template <typename T>
T PrevModN(T x, T n)
{
  ASSERT_GREATER(n, 0, ());
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

// Greatest Common Divisor.
template <typename Number,
          typename EnableIf = typename std::enable_if_t<std::is_integral<Number>::value, void>>
Number constexpr GCD(Number const a, Number const b)
{
  return b == 0 ? a : GCD(b, a % b);
}

// Least Common Multiple.
template <typename Number,
          typename EnableIf = typename std::enable_if_t<std::is_integral<Number>::value, void>>
Number constexpr LCM(Number const a, Number const b)
{
  return a / GCD(a, b) * b;
}

// Calculate hash for the pair of values.
template <typename T1, typename T2>
size_t Hash(T1 const & t1, T2 const & t2)
{
  /// @todo Probably, we need better hash for 2 integral types.
  return (std::hash<T1>()(t1) ^ (std::hash<T2>()(t2) << 1));
}
}  // namespace base
