#include "testing/testing.hpp"

#include "base/random.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

namespace random_tests
{
// Large enough that a value of a 1-byte type staying unseen has probability (255/256)^kDraws.
size_t constexpr kDraws = 100000;

template <class T>
void TestDistributionIntType()
{
  using ValueT = base::impl::DistributionIntT<T>;

  // Only these types are allowed as uniform_int_distribution's IntType, everything else is UB.
  static_assert(std::is_same_v<ValueT, short> || std::is_same_v<ValueT, int> || std::is_same_v<ValueT, long> ||
                std::is_same_v<ValueT, long long> || std::is_same_v<ValueT, unsigned short> ||
                std::is_same_v<ValueT, unsigned int> || std::is_same_v<ValueT, unsigned long> ||
                std::is_same_v<ValueT, unsigned long long>);

  // The mapped type must hold every value of T, so that the cast back in operator() stays exact.
  static_assert(std::is_signed_v<ValueT> == std::is_signed_v<T>);
  static_assert(std::numeric_limits<ValueT>::min() <= std::numeric_limits<T>::min());
  static_assert(std::numeric_limits<ValueT>::max() >= std::numeric_limits<T>::max());
}

template <class T>
void TestFullByteRange()
{
  int constexpr kMin = std::numeric_limits<T>::lowest();
  int constexpr kMax = std::numeric_limits<T>::max();
  static_assert(kMax - kMin + 1 == 256, "See the kDraws comment above.");

  base::UniformRandom<T> rand;
  std::vector<bool> seen(kMax - kMin + 1, false);
  for (size_t i = 0; i < kDraws; ++i)
  {
    int const v = rand();
    TEST_GREATER_OR_EQUAL(v, kMin, ());
    TEST_LESS_OR_EQUAL(v, kMax, ());
    seen[v - kMin] = true;
  }

  for (int v = kMin; v <= kMax; ++v)
    TEST(seen[v - kMin], ("Value is never generated:", v));
}

template <class T>
void TestFullIntegralRange()
{
  base::UniformRandom<T> rand;
  T lo = std::numeric_limits<T>::max();
  T hi = std::numeric_limits<T>::lowest();
  for (size_t i = 0; i < kDraws; ++i)
  {
    T const v = rand();
    lo = std::min(lo, v);
    hi = std::max(hi, v);
  }

  // Draws spread over the whole range hit the outer quarters with an overwhelming probability.
  TEST_LESS(lo, std::numeric_limits<T>::lowest() + std::numeric_limits<T>::max() / 4, ());
  TEST_GREATER(hi, std::numeric_limits<T>::max() - std::numeric_limits<T>::max() / 4, ());
}

template <class T>
void TestDefaultRealRange()
{
  base::UniformRandom<T> rand;
  for (size_t i = 0; i < kDraws; ++i)
  {
    T const v = rand();
    // The full floating point range is not a valid uniform_real_distribution range, because it
    // requires max - min <= numeric_limits<T>::max(); asking for it used to produce infinity.
    TEST(std::isfinite(v), (v));
    TEST_GREATER_OR_EQUAL(v, T{0}, ());
    // Not TEST_LESS: generate_canonical may return 1.0 (LWG 2524), and for float it does so about
    // once in 2^24 draws, because the largest mt19937 output rounds up to 2^32.
    TEST_LESS_OR_EQUAL(v, T{1}, ());
  }
}

// std::uniform_int_distribution accepts neither the 1-byte types nor the wide character ones, so
// UniformRandom has to map them onto the nearest type that it does accept.
UNIT_TEST(UniformRandom_DistributionIntType)
{
  TestDistributionIntType<bool>();
  TestDistributionIntType<char>();
  TestDistributionIntType<char8_t>();
  TestDistributionIntType<char16_t>();
  TestDistributionIntType<char32_t>();
  TestDistributionIntType<wchar_t>();
  TestDistributionIntType<int8_t>();
  TestDistributionIntType<uint8_t>();
  TestDistributionIntType<int16_t>();
  TestDistributionIntType<uint16_t>();
  TestDistributionIntType<int32_t>();
  TestDistributionIntType<uint32_t>();
  TestDistributionIntType<int64_t>();
  TestDistributionIntType<uint64_t>();
  TestDistributionIntType<size_t>();

  // Types that uniform_int_distribution rejects are the ones that have to be widened. Wide
  // character types are among them despite being larger than a byte.
  static_assert(!std::is_same_v<base::impl::DistributionIntT<int8_t>, int8_t>);
  static_assert(!std::is_same_v<base::impl::DistributionIntT<uint8_t>, uint8_t>);
  static_assert(!std::is_same_v<base::impl::DistributionIntT<char16_t>, char16_t>);
  static_assert(!std::is_same_v<base::impl::DistributionIntT<char32_t>, char32_t>);
  static_assert(std::is_same_v<base::impl::DistributionIntT<int>, int>);
  static_assert(std::is_same_v<base::impl::DistributionIntT<unsigned>, unsigned>);
}

UNIT_TEST(UniformRandom_AcceptedValueTypes)
{
  static_assert(base::impl::UniformRandomValue<int>);
  static_assert(base::impl::UniformRandomValue<uint8_t>);
  static_assert(base::impl::UniformRandomValue<char16_t>);
  static_assert(base::impl::UniformRandomValue<double>);

  // uniform_real_distribution takes float, double and long double only, and [rand.req.genl]
  // additionally requires the argument to be cv-unqualified.
  static_assert(!base::impl::UniformRandomValue<int const>);
  static_assert(!base::impl::UniformRandomValue<int &>);
  static_assert(!base::impl::UniformRandomValue<void *>);
  static_assert(!base::impl::UniformRandomValue<std::string>);
}

UNIT_TEST(UniformRandom_FullByteRange)
{
  TestFullByteRange<int8_t>();
  TestFullByteRange<uint8_t>();
  TestFullByteRange<char>();
}

UNIT_TEST(UniformRandom_FullIntegralRange)
{
  TestFullIntegralRange<int16_t>();
  TestFullIntegralRange<uint16_t>();
  TestFullIntegralRange<int32_t>();
  TestFullIntegralRange<uint32_t>();
  TestFullIntegralRange<int64_t>();
  TestFullIntegralRange<uint64_t>();
}

UNIT_TEST(UniformRandom_WideCharacterTypes)
{
  base::UniformRandom<char16_t> rand16;
  bool exceedsByte = false;
  for (size_t i = 0; i < kDraws; ++i)
  {
    uint32_t const v = rand16();
    TEST_LESS_OR_EQUAL(v, uint32_t{std::numeric_limits<char16_t>::max()}, ());
    exceedsByte = exceedsByte || v > std::numeric_limits<uint8_t>::max();
  }
  TEST(exceedsByte, ("char16_t draws are confined to a single byte"));

  base::UniformRandom<char32_t> rand32(U'Ѐ', U'ӿ');
  base::UniformRandom<wchar_t> randW(L'A', L'Z');
  for (size_t i = 0; i < kDraws; ++i)
  {
    uint32_t const v = rand32();
    TEST_GREATER_OR_EQUAL(v, uint32_t{U'Ѐ'}, ());
    TEST_LESS_OR_EQUAL(v, uint32_t{U'ӿ'}, ());

    int const w = randW();
    TEST_GREATER_OR_EQUAL(w, int{L'A'}, ());
    TEST_LESS_OR_EQUAL(w, int{L'Z'}, ());
  }
}

UNIT_TEST(UniformRandom_DefaultRealRange)
{
  TestDefaultRealRange<float>();
  TestDefaultRealRange<double>();
}

UNIT_TEST(UniformRandom_ExplicitLimits)
{
  base::UniformRandom<int> fixed(7, 7);
  for (size_t i = 0; i < 100; ++i)
    TEST_EQUAL(fixed(), 7, ());

  // Both ends of an integral range are inclusive.
  base::UniformRandom<int> coin(0, 1);
  bool zero = false, one = false;
  for (size_t i = 0; i < 100; ++i)
  {
    int const v = coin();
    TEST_GREATER_OR_EQUAL(v, 0, ());
    TEST_LESS_OR_EQUAL(v, 1, ());
    zero = zero || v == 0;
    one = one || v == 1;
  }
  TEST(zero && one, ("Both bounds should be reachable"));

  coin.SetLimits(100, 200);
  for (size_t i = 0; i < kDraws; ++i)
  {
    int const v = coin();
    TEST_GREATER_OR_EQUAL(v, 100, ());
    TEST_LESS_OR_EQUAL(v, 200, ());
  }

  // Widening a 1-byte type must not let draws escape the requested limits.
  base::UniformRandom<uint8_t> byte(250, 255);
  for (size_t i = 0; i < kDraws; ++i)
  {
    int const v = byte();
    TEST_GREATER_OR_EQUAL(v, 250, ());
    TEST_LESS_OR_EQUAL(v, 255, ());
  }

  base::UniformRandom<double> real(-1.0, 1.0);
  for (size_t i = 0; i < kDraws; ++i)
  {
    double const v = real();
    TEST_GREATER_OR_EQUAL(v, -1.0, ());
    TEST_LESS_OR_EQUAL(v, 1.0, ());
  }
}
}  // namespace random_tests
