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
  double constexpr pi2 = pi / 2.;
  double constexpr pi4 = pi / 4.;
  double constexpr twicePi = 2. * pi;

  template <class T> T sqr(T t) { return (t*t); }
}  // namespace math

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
  static_assert(std::is_floating_point<TFloat>::value, "");
  static_assert(std::numeric_limits<TFloat>::is_iec559, "");

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
  return fabs(x - y) < eps * std::max(fabs(x), fabs(y));
}

// Returns true if x and y are equal up to the absolute or relative difference eps.
template <typename TFloat>
inline bool AlmostEqualAbsOrRel(TFloat x, TFloat y, TFloat eps)
{
  return AlmostEqualAbs(x, y, eps) || AlmostEqualRel(x, y, eps);
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

template <typename T>
inline T clamp(T const x, T const xmin, T const xmax)
{
  if (x > xmax)
    return xmax;
  if (x < xmin)
    return xmin;
  return x;
}

template <typename T>
inline T cyclicClamp(T const x, T const xmin, T const xmax)
{
  if (x > xmax)
    return xmin;
  if (x < xmin)
    return xmax;
  return x;
}

template <typename T> inline bool between_s(T const a, T const b, T const x)
{
  return (a <= x && x <= b);
}
template <typename T> inline bool between_i(T const a, T const b, T const x)
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

template <typename Number,
          typename EnableIf = typename std::enable_if<
            std::is_integral<Number>::value, void>::type>
// Greatest Common Divisor
Number constexpr GCD(Number const a, Number const b) { return b == 0 ? a : GCD(b, a % b); }

template <typename Number,
          typename EnableIf = typename std::enable_if<
            std::is_integral<Number>::value, void>::type>
// Lowest Common Multiple.
Number constexpr LCM(Number const a, Number const b) { return a / GCD(a, b) * b; }

/// Calculate hash for the pair of values.
template <typename T1, typename T2>
size_t Hash(T1 const & t1, T2 const & t2)
{
  /// @todo Probably, we need better hash for 2 integral types.
  return (std::hash<T1>()(t1) ^ (std::hash<T2>()(t2) << 1));
}

template <typename Number,
          typename EnableIf = typename std::enable_if<
            std::is_integral<Number>::value || std::is_floating_point<Number>::value,
            void>::type>
int constexpr Sign(Number const number) noexcept
{
  return number == 0 ? 0 : number > 0 ? 1 : -1;
}
}  // namespace my
