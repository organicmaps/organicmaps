#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

// Formats a float/double with up to `precision` digits after the decimal point,
// stripping trailing zeroes (and the dot itself if nothing follows it).
// Does NOT depend on locale, std::to_chars, or snprintf - pure integer/bignum arithmetic.
// Fast path: integer arithmetic for values whose fixed-point representation
// fits in uint64_t (~99% of practical inputs).
// General path: modf-based split with a base-10^9 bignum for very large integer parts.

namespace base
{
namespace detail
{
// Write a uint64_t into buf in decimal, return number of chars written.
inline int U64ToBuf(char * buf, uint64_t v) noexcept
{
  if (v == 0)
  {
    buf[0] = '0';
    return 1;
  }
  int n = 0;
  while (v)
  {
    buf[n++] = '0' + static_cast<char>(v % 10);
    v /= 10;
  }
  for (int i = 0, j = n - 1; i < j; ++i, --j)
  {
    char const t = buf[i];
    buf[i] = buf[j];
    buf[j] = t;
  }
  return n;
}

inline constexpr uint64_t kPow10[] = {
    1ull,
    10ull,
    100ull,
    1'000ull,
    10'000ull,
    100'000ull,
    1'000'000ull,
    10'000'000ull,
    100'000'000ull,
    1'000'000'000ull,
    10'000'000'000ull,
    100'000'000'000ull,
    1'000'000'000'000ull,
    10'000'000'000'000ull,
    100'000'000'000'000ull,
    1'000'000'000'000'000ull,
    10'000'000'000'000'000ull,
    100'000'000'000'000'000ull,
    1'000'000'000'000'000'000ull,
};

inline constexpr double kPow10d[] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18,
};

// 2^53: largest integer where every integer <= it is exactly representable as a double.
inline constexpr double kMaxExactInt = 9007199254740992.0;
// Approximate uint64_t max as a double threshold for the fast U64ToBuf path.
inline constexpr double kMaxU64Approx = 1.8e19;

// ---- Decimal bignum for converting very large integers to decimal ----
// Stores a non-negative integer in base 10^9 (each word is 0..999'999'999).
// Sufficient for any IEEE 754 double integer part (up to ~309 decimal digits).
struct DecBignum
{
  static constexpr uint32_t kBase = 1'000'000'000u;
  static constexpr int kMaxWords = 36;  // 36 * 9 = 324 >= 309 digits
  uint32_t words[kMaxWords]{};
  int size = 0;

  void init(uint64_t v)
  {
    size = 0;
    if (v == 0)
    {
      words[0] = 0;
      size = 1;
      return;
    }
    while (v > 0)
    {
      words[size++] = static_cast<uint32_t>(v % kBase);
      v /= kBase;
    }
  }

  void Multiply(uint32_t factor)
  {
    uint64_t carry = 0;
    for (int i = 0; i < size; ++i)
    {
      uint64_t const val = static_cast<uint64_t>(words[i]) * factor + carry;
      words[i] = static_cast<uint32_t>(val % kBase);
      carry = val / kBase;
    }
    while (carry > 0)
    {
      words[size++] = static_cast<uint32_t>(carry % kBase);
      carry /= kBase;
    }
  }

  // Multiply by 2^bits using chunks of 2^29 (fits in uint32_t).
  void ShiftLeft(int bits)
  {
    while (bits >= 29)
    {
      Multiply(1u << 29);
      bits -= 29;
    }
    if (bits > 0)
      Multiply(1u << bits);
  }

  int ToChars(char * buf) const
  {
    if (size == 0 || (size == 1 && words[0] == 0))
    {
      buf[0] = '0';
      return 1;
    }
    char * p = buf;
    // Most significant word without leading zeros.
    p += U64ToBuf(p, words[size - 1]);
    // Remaining words: exactly 9 digits each with leading zeros.
    for (int w = size - 2; w >= 0; --w)
    {
      uint32_t val = words[w];
      for (int d = 8; d >= 0; --d)
      {
        p[d] = '0' + static_cast<char>(val % 10);
        val /= 10;
      }
      p += 9;
    }
    return static_cast<int>(p - buf);
  }
};

// Convert a large double integer (>= 1.8e19, finite) to decimal via IEEE 754 decomposition.
inline int LargeDoubleIntToChars(char * buf, double v)
{
  uint64_t bits;
  std::memcpy(&bits, &v, sizeof(bits));
  int const biased_exp = static_cast<int>((bits >> 52) & 0x7FF);
  uint64_t const significand = (bits & 0x000F'FFFF'FFFF'FFFFull) | 0x0010'0000'0000'0000ull;
  int const exponent = biased_exp - 1023 - 52;

  DecBignum bn;
  bn.init(significand);
  if (exponent > 0)
    bn.ShiftLeft(exponent);
  return bn.ToChars(buf);
}

// Write fractional digits with trailing-zero stripping.
// Returns number of chars written (0 if frac is zero or precision is 0).
inline int WriteFrac(char * p, uint64_t frac, uint8_t precision) noexcept
{
  if (precision == 0 || frac == 0)
    return 0;

  char * start = p;
  *p++ = '.';

  char ftmp[20];
  int const flen = U64ToBuf(ftmp, frac);

  // Leading zeros: precision - flen of them.
  for (int i = 0; i < static_cast<int>(precision) - flen; ++i)
    *p++ = '0';
  for (int i = 0; i < flen; ++i)
    *p++ = ftmp[i];

  // Strip trailing '0's and a lone '.'.
  while (*(p - 1) == '0')
    --p;
  if (*(p - 1) == '.')
    --p;

  return static_cast<int>(p - start);
}

}  // namespace detail

template <typename T>
  requires std::is_floating_point_v<T>
[[nodiscard]] std::string FloatToString(T value, uint8_t precision = 6)
{
  // --- special values ---
  if (std::isnan(value))
    return "NaN";
  if (std::isinf(value))
    return value < T{0} ? "-inf" : "inf";

  bool neg = std::signbit(value);
  double v = neg ? -static_cast<double>(value) : static_cast<double>(value);

  precision = (precision > 18 ? 18 : precision);

  // Buffer: '-'(1) + integer digits(<=309) + '.'(1) + precision(<=18) = 329 max
  char buf[350];
  char * p = buf;

  // Fast path: v * 10^precision fits exactly in a double integer.
  double const scaled = std::round(v * detail::kPow10d[precision]);

  if (scaled < detail::kMaxExactInt)
  {
    uint64_t fixed = static_cast<uint64_t>(scaled);

    // Suppress negative zero.
    if (neg && fixed == 0)
      neg = false;
    if (neg)
      *p++ = '-';

    uint64_t const divisor = detail::kPow10[precision];
    uint64_t const int_part = fixed / divisor;
    uint64_t const frac = fixed % divisor;

    p += detail::U64ToBuf(p, int_part);
    p += detail::WriteFrac(p, frac, precision);

    return std::string(buf, static_cast<size_t>(p - buf));
  }

  // ---- general path: split into integer + fractional parts ----
  double int_val;
  uint64_t frac_scaled = 0;

  if (precision > 0)
  {
    double const frac_val = std::modf(v, &int_val);

    if (precision <= 9)
    {
      // frac_val * 10^9 < 10^9 < 2^30: fits exactly in a double.
      double const fs = std::round(frac_val * detail::kPow10d[precision]);
      if (fs >= detail::kPow10d[precision])
      {
        frac_scaled = 0;
        int_val += 1.0;
      }
      else
      {
        frac_scaled = static_cast<uint64_t>(fs);
      }
    }
    else
    {
      // For precision 10-18, a single frac_val * 10^precision can exceed 2^53
      // and lose low decimal digits.  Split into two 9-digit blocks so each
      // intermediate value stays below 10^9 < 2^30.
      int const pLo = static_cast<int>(precision) - 9;

      double const fHi = frac_val * detail::kPow10d[9];  // < 10^9, exact as double
      double fHi_int;
      double const fHi_rem = std::modf(fHi, &fHi_int);

      double fLo = std::round(fHi_rem * detail::kPow10d[pLo]);
      auto hi = static_cast<uint64_t>(fHi_int);
      uint64_t lo;

      if (fLo >= detail::kPow10d[pLo])
      {
        lo = 0;
        ++hi;
      }
      else
      {
        lo = static_cast<uint64_t>(fLo);
      }

      // Handle carry from fractional part into integer part.
      if (hi >= detail::kPow10[9])
      {
        frac_scaled = 0;
        int_val += 1.0;
      }
      else
      {
        frac_scaled = hi * detail::kPow10[pLo] + lo;
      }
    }
  }
  else
  {
    int_val = std::round(v);
  }

  // Suppress negative zero.
  if (neg && int_val == 0.0 && frac_scaled == 0)
    neg = false;
  if (neg)
    *p++ = '-';

  // Format integer part.
  if (int_val < detail::kMaxU64Approx)
    p += detail::U64ToBuf(p, static_cast<uint64_t>(int_val));
  else
    p += detail::LargeDoubleIntToChars(p, int_val);

  // Format fractional part.
  p += detail::WriteFrac(p, frac_scaled, precision);

  return std::string(buf, static_cast<size_t>(p - buf));
}
}  // namespace base
