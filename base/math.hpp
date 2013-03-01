#pragma once
#include "../base/base.hpp"
#include "../base/assert.hpp"

#include "../std/cmath.hpp"
#include "../std/limits.hpp"
#include "../std/type_traits.hpp"

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
// See http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm for details.
template <typename FloatT> bool AlmostEqual(FloatT x, FloatT y, unsigned int maxULPs = 256)
{
  STATIC_ASSERT(is_floating_point<FloatT>::value);
  STATIC_ASSERT(numeric_limits<FloatT>::is_iec559);
  STATIC_ASSERT(!numeric_limits<FloatT>::is_exact);
  STATIC_ASSERT(!numeric_limits<FloatT>::is_integer);

  // Make sure maxUlps is non-negative and small enough that the
  // default NAN won't compare as equal to anything.
  ASSERT_LESS(maxULPs, 4 * 1024 * 1024, ());

  int const bits = 8 * sizeof(FloatT);
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

  return (diff <= maxULPs);
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

template <typename T>
void Merge(T const & x0, T const & x1, T const & x2, T const & x3, T & x4, T & x5)
{
  ASSERT(IsIntersect(x0, x1, x2, x3), ());
  x4 = min(x0, x2);
  x5 = max(x3, x1);
}

template <typename T>
size_t MergeSorted(T const * a, size_t as, T const * b, size_t bs, T * c, size_t cs)
{
  T const * arrs [] = {a, b};
  size_t sizes[] = {as, bs};
  size_t idx[] = {0, 0};

  int i = 0;
  int j = 1;
  int k = 0;

  if ((sizes[i] == 0) && (sizes[j] == 0))
    return 0;

  if ((sizes[i] == 0) && (sizes[j] != 0))
    swap(i, j);

  while (true)
  {
    /// selecting start of new interval
    if ((idx[j] != sizes[j]) && (arrs[i][idx[i]] > arrs[j][idx[j]]))
      swap(i, j);

    c[k]     = arrs[i][idx[i]];
    c[k + 1] = arrs[i][idx[i] + 1];

    idx[i] += 2;

    while (true)
    {
      if (idx[j] == sizes[j])
        break;

      bool merged = false;

      while (IsIntersect(c[k], c[k + 1], arrs[j][idx[j]], arrs[j][idx[j] + 1]))
      {
        merged = true;
        Merge(c[k], c[k + 1], arrs[j][idx[j]], arrs[j][idx[j] + 1], c[k], c[k + 1]);
        idx[j] += 2;
        if (idx[j] == sizes[j])
          break;
      }

      if (!merged)
        break;

      swap(i, j);
    }

    /// here idx[i] and idx[j] should point to intervals that
    /// aren't overlapping c[k], c[k + 1]

    k += 2;

    if ((idx[i] == sizes[i]) && (idx[j] == sizes[j]))
      break;

    if (idx[i] == sizes[i])
      swap(i, j);
  }

  return k;
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

}
