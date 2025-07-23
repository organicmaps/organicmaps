#include "base/rolling_hash.hpp"
#include "testing/benchmark.hpp"
#include "testing/testing.hpp"

#include "base/base.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

namespace
{

template <class RollingHasherT>
void SmokeTest1RollingHasher()
{
  typedef typename RollingHasherT::hash_type hash_type;
  RollingHasherT hash;
  hash_type const h0 = hash.Init("a", 1);
  TEST_EQUAL(h0, hash.Scroll('a', 'a'), (sizeof(hash_type)));
}

template <class RollingHasherT>
void SmokeTest2RollingHasher()
{
  typedef typename RollingHasherT::hash_type hash_type;
  RollingHasherT hash;
  hash_type const hAB = hash.Init("ab", 2);
  hash_type const hBA = hash.Scroll('a', 'a');
  hash_type const hAB1 = hash.Scroll('b', 'b');
  TEST_EQUAL(hAB, hAB1, (sizeof(hash_type)));
  hash_type const hBA1 = hash.Scroll('a', 'a');
  TEST_EQUAL(hBA, hBA1, (sizeof(hash_type)));
}

template <class RollingHasherT>
void TestRollingHasher()
{
  SmokeTest1RollingHasher<RollingHasherT>();
  SmokeTest2RollingHasher<RollingHasherT>();
  //                 01234567890123
  char const s[] = "abcdefghaabcde";
  size_t const len = ARRAY_SIZE(s) - 1;
  for (uint32_t size = 1; size <= 6; ++size)
  {
    typedef typename RollingHasherT::hash_type hash_type;
    RollingHasherT hash;
    std::vector<hash_type> hashes;
    hashes.push_back(hash.Init(static_cast<char const *>(s), size));
    for (uint32_t i = size; i < len; ++i)
      hashes.push_back(hash.Scroll(s[i - size], s[i]));
    TEST_EQUAL(hashes.size(), len - size + 1, (size, len, sizeof(hash_type)));
    switch (size)
    {
    case 6:
    {
      // Test that there are no collisions.
      sort(hashes.begin(), hashes.end());
      TEST(hashes.end() == unique(hashes.begin(), hashes.end()), (size, hashes));
    }
    break;
    case 1:
    {
      TEST_EQUAL(hashes[0], hashes[8], (size, len, sizeof(hash_type)));
      TEST_EQUAL(hashes[0], hashes[9], (size, len, sizeof(hash_type)));
      TEST_EQUAL(hashes[1], hashes[10], (size, len, sizeof(hash_type)));
      TEST_EQUAL(hashes[2], hashes[11], (size, len, sizeof(hash_type)));
      TEST_EQUAL(hashes[3], hashes[12], (size, len, sizeof(hash_type)));
      TEST_EQUAL(hashes[4], hashes[13], (size, len, sizeof(hash_type)));
    }
    break;
    default:
    {
      for (unsigned int i = 0; i < 6 - size; ++i)
        TEST_EQUAL(hashes[i], hashes[i + 9], (i, size, len, sizeof(hash_type)));
      sort(hashes.begin(), hashes.end());
      TEST((hashes.end() - (6 - size)) == unique(hashes.begin(), hashes.end()), (size, hashes));
    }
    }
  }
}

}  // namespace

UNIT_TEST(RabinKarpRollingHasher32)
{
  TestRollingHasher<RabinKarpRollingHasher32>();
}

UNIT_TEST(RabinKarpRollingHasher64)
{
  TestRollingHasher<RabinKarpRollingHasher64>();
}

UNIT_TEST(RollingHasher32)
{
  TestRollingHasher<RollingHasher32>();
}

UNIT_TEST(RollingHasher64)
{
  TestRollingHasher<RollingHasher64>();
}

#ifndef DEBUG
BENCHMARK_TEST(RollingHasher32)
{
  RollingHasher32 hash;
  hash.Init("abcdefghijklmnopq", 17);
  BENCHMARK_N_TIMES(IF_DEBUG_ELSE(500000, 20000000), 0.2)
  {
    FORCE_USE_VALUE(hash.Scroll(static_cast<uint32_t>('a') + benchmark.Iteration(),
                                static_cast<uint32_t>('r') + benchmark.Iteration()));
  }
}

BENCHMARK_TEST(RollingHasher64)
{
  RollingHasher64 hash;
  hash.Init("abcdefghijklmnopq", 17);
  BENCHMARK_N_TIMES(IF_DEBUG_ELSE(500000, 10000000), 0.3)
  {
    FORCE_USE_VALUE(hash.Scroll(static_cast<uint64_t>('a') + benchmark.Iteration(),
                                static_cast<uint64_t>('r') + benchmark.Iteration()));
  }
}
#endif
