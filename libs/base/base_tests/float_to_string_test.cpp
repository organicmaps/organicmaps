#include "testing/testing.hpp"

#include "base/float_to_string.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace float_to_string_test
{
using base::FloatToString;
// --- Special values ---
/* -ffast-math changes the NaN and infinity float representation in a weird way, commented as not critical for our
cases. UNIT_TEST(FloatToString_NaN)
{
  TEST_EQUAL(FloatToString(std::numeric_limits<double>::quiet_NaN()), "NaN", ());
  TEST_EQUAL(FloatToString(std::numeric_limits<float>::quiet_NaN()), "NaN", ());
}

UNIT_TEST(FloatToString_Infinity)
{
  TEST_EQUAL(FloatToString(std::numeric_limits<double>::infinity()), "inf", ());
  TEST_EQUAL(FloatToString(-std::numeric_limits<double>::infinity()), "-inf", ());
  TEST_EQUAL(FloatToString(std::numeric_limits<float>::infinity()), "inf", ());
  TEST_EQUAL(FloatToString(-std::numeric_limits<float>::infinity()), "-inf", ());
}
*/
UNIT_TEST(FloatToString_NegativeZero)
{
  TEST_EQUAL(FloatToString(-0.0), "0", ());
  TEST_EQUAL(FloatToString(-0.0, 0), "0", ());
  TEST_EQUAL(FloatToString(-0.0, 2), "0", ());
  TEST_EQUAL(FloatToString(-0.0f), "0", ());
}

// --- Zero ---

UNIT_TEST(FloatToString_Zero)
{
  TEST_EQUAL(FloatToString(0.0), "0", ());
  TEST_EQUAL(FloatToString(0.0, 0), "0", ());
  TEST_EQUAL(FloatToString(0.0, 1), "0", ());
  TEST_EQUAL(FloatToString(0.0, 18), "0", ());
  TEST_EQUAL(FloatToString(0.0f, 6), "0", ());
}

// --- Basic values ---

UNIT_TEST(FloatToString_BasicPositive)
{
  TEST_EQUAL(FloatToString(1.0), "1", ());
  TEST_EQUAL(FloatToString(1.0, 0), "1", ());
  TEST_EQUAL(FloatToString(42.0, 6), "42", ());
  TEST_EQUAL(FloatToString(123456789.0, 0), "123456789", ());
}

UNIT_TEST(FloatToString_BasicNegative)
{
  TEST_EQUAL(FloatToString(-1.0), "-1", ());
  TEST_EQUAL(FloatToString(-42.0, 6), "-42", ());
  TEST_EQUAL(FloatToString(-123.456, 3), "-123.456", ());
}

UNIT_TEST(FloatToString_Fractional)
{
  TEST_EQUAL(FloatToString(3.14, 2), "3.14", ());
  TEST_EQUAL(FloatToString(0.5, 1), "0.5", ());
  TEST_EQUAL(FloatToString(0.25, 2), "0.25", ());
  TEST_EQUAL(FloatToString(0.125, 3), "0.125", ());
  TEST_EQUAL(FloatToString(0.001, 3), "0.001", ());
}

// --- Trailing zero stripping ---

UNIT_TEST(FloatToString_TrailingZeroStrip)
{
  TEST_EQUAL(FloatToString(1.0, 6), "1", ());
  TEST_EQUAL(FloatToString(1.5, 6), "1.5", ());
  TEST_EQUAL(FloatToString(1.50, 6), "1.5", ());
  TEST_EQUAL(FloatToString(1.25, 4), "1.25", ());
  TEST_EQUAL(FloatToString(100.0, 10), "100", ());
  TEST_EQUAL(FloatToString(1.100, 3), "1.1", ());
}

// --- Precision 0 (rounding to integer) ---

UNIT_TEST(FloatToString_Precision0)
{
  TEST_EQUAL(FloatToString(3.3, 0), "3", ());
  TEST_EQUAL(FloatToString(3.7, 0), "4", ());
  TEST_EQUAL(FloatToString(0.5, 0), "1", ());
  TEST_EQUAL(FloatToString(-3.7, 0), "-4", ());
  TEST_EQUAL(FloatToString(-0.3, 0), "0", ());
  TEST_EQUAL(FloatToString(99.5, 0), "100", ());
}

// --- Rounding ---

UNIT_TEST(FloatToString_Rounding)
{
  TEST_EQUAL(FloatToString(1.005, 2), "1", ());     // 1.005 as double is < 1.005
  TEST_EQUAL(FloatToString(1.235, 2), "1.24", ());  // 1.235 as double is > 1.235
  TEST_EQUAL(FloatToString(9.999, 2), "10", ());
  TEST_EQUAL(FloatToString(0.999, 2), "1", ());
  TEST_EQUAL(FloatToString(0.001, 2), "0", ());
  TEST_EQUAL(FloatToString(0.009, 2), "0.01", ());
}

// --- Carry propagation in fractional rounding ---

UNIT_TEST(FloatToString_FracCarry)
{
  // Fractional part rounds up to 10^precision → carry into integer part.
  TEST_EQUAL(FloatToString(9.9999, 3), "10", ());
  TEST_EQUAL(FloatToString(99.9999, 3), "100", ());
  TEST_EQUAL(FloatToString(0.9999, 3), "1", ());
}

// --- Precision clamping ---

UNIT_TEST(FloatToString_PrecisionClamp)
{
  // Precision > 18 clamped to 18.
  TEST_EQUAL(FloatToString(1.0, 20), "1", ());
  TEST_EQUAL(FloatToString(1.0, 100), "1", ());
}

// --- Float input ---

UNIT_TEST(FloatToString_Float)
{
  TEST_EQUAL(FloatToString(1.5f), "1.5", ());
  TEST_EQUAL(FloatToString(0.0f), "0", ());
  TEST_EQUAL(FloatToString(-1.5f, 1), "-1.5", ());
  TEST_EQUAL(FloatToString(0.1f, 1), "0.1", ());
}

// --- High precision ---

UNIT_TEST(FloatToString_HighPrecision)
{
  TEST_EQUAL(FloatToString(1.0, 18), "1", ());
  TEST_EQUAL(FloatToString(0.0, 18), "0", ());

  // With precision 18, pi should have all 18 digits.
  std::string pi18 = FloatToString(3.141592653589793, 18);
  // Must start with the correct significant digits.
  TEST(pi18.substr(0, 10) == "3.14159265", (pi18));
  TEST(pi18.size() > 10, (pi18));
}

// --- Very small values ---

UNIT_TEST(FloatToString_VerySmall)
{
  TEST_EQUAL(FloatToString(1e-10, 6), "0", ());
  TEST_EQUAL(FloatToString(1e-7, 7), "0.0000001", ());
  TEST_EQUAL(FloatToString(1e-7, 6), "0", ());
  TEST_EQUAL(FloatToString(5e-7, 6), "0.000001", ());

  // Subnormals.
  double subnormal = std::numeric_limits<double>::denorm_min();
  TEST_EQUAL(FloatToString(subnormal, 6), "0", ());
  TEST_EQUAL(FloatToString(subnormal, 0), "0", ());
}

// --- Large values (general path: scaled >= 9e18) ---

UNIT_TEST(FloatToString_LargeValues)
{
  // 1e20 is exactly representable.
  TEST_EQUAL(FloatToString(1e20, 0), "100000000000000000000", ());
  TEST_EQUAL(FloatToString(1e20, 6), "100000000000000000000", ());

  // Negative large.
  TEST_EQUAL(FloatToString(-1e20, 0), "-100000000000000000000", ());

  // 2^64 = 18446744073709551616.
  double pow2_64 = std::ldexp(1.0, 64);
  TEST_EQUAL(FloatToString(pow2_64, 0), "18446744073709551616", ());
}

// --- Very large values (bignum path) ---

UNIT_TEST(FloatToString_VeryLargeValues)
{
  // 1eN as a double may be slightly above or below 10^N, giving N+1 or N digits.
  std::string s = FloatToString(1e50, 0);
  TEST(s.size() >= 50 && s.size() <= 51, (s, s.size()));

  s = FloatToString(1e100, 0);
  TEST(s.size() >= 100 && s.size() <= 101, (s, s.size()));

  s = FloatToString(1e200, 0);
  TEST(s.size() >= 200 && s.size() <= 201, (s, s.size()));

  // DBL_MAX ≈ 1.8e308, so 309 digits.
  s = FloatToString(std::numeric_limits<double>::max(), 0);
  TEST(s.size() == 309, (s, s.size()));
  TEST(s.substr(0, 17) == "17976931348623157", (s));
}

// --- Coordinates (typical use case for maps) ---

UNIT_TEST(FloatToString_Coordinates)
{
  // Latitude/longitude with 6 decimal places.
  TEST_EQUAL(FloatToString(55.755833, 6), "55.755833", ());
  TEST_EQUAL(FloatToString(37.617222, 6), "37.617222", ());
  TEST_EQUAL(FloatToString(-33.868820, 6), "-33.86882", ());
  TEST_EQUAL(FloatToString(151.209296, 6), "151.209296", ());

  // With 8 decimal places.
  TEST_EQUAL(FloatToString(55.75583300, 8), "55.755833", ());
  TEST_EQUAL(FloatToString(37.61722200, 8), "37.617222", ());
}

// --- General path with fractional parts ---

UNIT_TEST(FloatToString_GeneralPathWithFrac)
{
  // precision=18 pushes most values into the general path (since v*10^18 > 9e18 for v >= 9).
  TEST_EQUAL(FloatToString(10.5, 18), "10.5", ());
  TEST_EQUAL(FloatToString(100.25, 18), "100.25", ());
  TEST_EQUAL(FloatToString(1000.125, 18), "1000.125", ());
}

// --- Negative small values rounding to zero ---

UNIT_TEST(FloatToString_NegSmallToZero)
{
  TEST_EQUAL(FloatToString(-0.001, 2), "0", ());
  TEST_EQUAL(FloatToString(-0.0001, 3), "0", ());
  TEST_EQUAL(FloatToString(-1e-10, 6), "0", ());
}

// --- Powers of 10 ---

UNIT_TEST(FloatToString_PowersOf10)
{
  TEST_EQUAL(FloatToString(0.1, 1), "0.1", ());
  TEST_EQUAL(FloatToString(0.01, 2), "0.01", ());
  TEST_EQUAL(FloatToString(0.001, 3), "0.001", ());
  TEST_EQUAL(FloatToString(10.0, 0), "10", ());
  TEST_EQUAL(FloatToString(100.0, 0), "100", ());
  TEST_EQUAL(FloatToString(1000.0, 0), "1000", ());
}

// --- Integer values ---

UNIT_TEST(FloatToString_Integers)
{
  TEST_EQUAL(FloatToString(0.0, 6), "0", ());
  TEST_EQUAL(FloatToString(1.0, 6), "1", ());
  TEST_EQUAL(FloatToString(999.0, 6), "999", ());
  TEST_EQUAL(FloatToString(1000000.0, 6), "1000000", ());
  TEST_EQUAL(FloatToString(1e15, 0), "1000000000000000", ());
}

// --- Bignum correctness: known exact powers of 2 ---

UNIT_TEST(FloatToString_ExactPowersOf2)
{
  // 2^53 = 9007199254740992 (fits in uint64_t, fast path)
  TEST_EQUAL(FloatToString(9007199254740992.0, 0), "9007199254740992", ());

  // 2^64 = 18446744073709551616 (bignum path)
  TEST_EQUAL(FloatToString(std::ldexp(1.0, 64), 0), "18446744073709551616", ());

  // 2^80 = 1208925819614629174706176
  TEST_EQUAL(FloatToString(std::ldexp(1.0, 80), 0), "1208925819614629174706176", ());

  // 2^100 = 1267650600228229401496703205376
  TEST_EQUAL(FloatToString(std::ldexp(1.0, 100), 0), "1267650600228229401496703205376", ());
}

// --- Precision loss near 2^53 boundary (fast-path cutoff regression) ---

UNIT_TEST(FloatToString_LargeIntegerWithPrecision)
{
  // 2^53 - 1 is exactly representable; with precision=1 the scaled value exceeds 2^53
  TEST_EQUAL(FloatToString(9007199254740991.0, 1), "9007199254740991", ());
  TEST_EQUAL(FloatToString(9007199254740991.0, 2), "9007199254740991", ());

  // A value just above 2^53 with precision that pushes it well past 2^53.
  TEST_EQUAL(FloatToString(1e16, 2), "10000000000000000", ());
  TEST_EQUAL(FloatToString(1e17, 1), "100000000000000000", ());
}

// --- High-precision fractional digits (general-path scaling regression) ---

UNIT_TEST(FloatToString_HighPrecisionExactFraction)
{
  // 25.999755859375 = 25 + 65535/65536, exactly representable.
  TEST_EQUAL(FloatToString(25.999755859375, 18), "25.999755859375", ());

  // 0.5 is exact in binary; high precision must not invent trailing digits.
  TEST_EQUAL(FloatToString(0.5, 18), "0.5", ());
  TEST_EQUAL(FloatToString(100.25, 18), "100.25", ());
  TEST_EQUAL(FloatToString(1000.125, 18), "1000.125", ());

  // Negative value with exact binary fraction.
  TEST_EQUAL(FloatToString(-0.5, 18), "-0.5", ());
  TEST_EQUAL(FloatToString(-25.999755859375, 18), "-25.999755859375", ());
}

// --- General-path fractional carry with high precision ---

UNIT_TEST(FloatToString_HighPrecisionFracCarry)
{
  // Fractional part that rounds up to 1.0 at high precision,
  // requiring carry into the integer part.
  TEST_EQUAL(FloatToString(9.9999, 3), "10", ());
  TEST_EQUAL(FloatToString(99.9999, 3), "100", ());

  // Large value with precision=10 (general path), fractional carry.
  TEST_EQUAL(FloatToString(999999999.9999999, 3), "1000000000", ());
}

}  // namespace float_to_string_test
