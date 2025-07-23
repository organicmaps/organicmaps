#include "testing/testing.hpp"

#include "coding/fixed_bits_ddvector.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <initializer_list>
#include <random>
#include <utility>

using namespace std;

namespace
{
template <size_t Bits>
void TestWithData(vector<uint32_t> const & lst)
{
  using TVector = FixedBitsDDVector<Bits, MemReader>;
  using TBuffer = vector<uint8_t>;
  using TWriter = MemWriter<TBuffer>;

  TBuffer buf;
  {
    TWriter writer(buf);
    typename TVector::template Builder<TWriter> builder(writer);

    uint32_t optCount = 0;
    uint32_t const optBound = (1 << Bits) - 2;

    for (uint32_t v : lst)
    {
      if (v < optBound)
        ++optCount;

      builder.PushBack(v);
    }

    pair<uint32_t, uint32_t> expected(optCount, lst.size());
    TEST_EQUAL(builder.GetCount(), expected, ());
  }

  MemReader reader(buf.data(), buf.size());
  auto const vec = TVector::Create(reader);

  uint32_t i = 0;
  for (uint32_t actual : lst)
  {
    uint32_t expected;
    TEST(vec->Get(i, expected), ());
    TEST_EQUAL(expected, actual, ());
    ++i;
  }
}
}  // namespace

UNIT_TEST(FixedBitsDDVector_Smoke)
{
  TestWithData<3>({0, 3, 6});
  TestWithData<3>({7, 20, 50});
  TestWithData<3>({1, 0, 4, 30, 5, 3, 6, 7, 2, 8, 0});
}

UNIT_TEST(FixedBitsDDVector_Rand)
{
  vector<uint32_t> v;

  default_random_engine gen;
  uniform_int_distribution<uint32_t> distribution(0, 1000);

  size_t constexpr kMaxCount = 1000;
  for (size_t i = 0; i < kMaxCount; ++i)
    v.push_back(distribution(gen));

  TestWithData<3>(v);
  TestWithData<4>(v);
  TestWithData<5>(v);
  TestWithData<6>(v);
  TestWithData<7>(v);
  TestWithData<8>(v);
  TestWithData<9>(v);
}
