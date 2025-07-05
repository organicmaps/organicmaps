#include "testing/testing.hpp"

#include "coding/byte_stream.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <vector>

using namespace std;

namespace
{
template <typename T>
void TestVarUint(T const x)
{
  vector<unsigned char> data;
  PushBackByteSink<vector<uint8_t>> dst(data);
  WriteVarUint(dst, x);

  ArrayByteSource src(&data[0]);
  TEST_EQUAL(ReadVarUint<T>(src), x, ());

  size_t const bytesRead = src.PtrUint8() - data.data();
  TEST_EQUAL(bytesRead, data.size(), (x));
}

template <typename T>
void TestVarInt(T const x)
{
  vector<uint8_t> data;
  PushBackByteSink<vector<uint8_t>> dst(data);
  WriteVarInt(dst, x);

  ArrayByteSource src(&data[0]);
  TEST_EQUAL(ReadVarInt<T>(src), x, ());

  size_t const bytesRead = src.PtrUint8() - data.data();
  TEST_EQUAL(bytesRead, data.size(), (x));
}
}  // namespace

UNIT_TEST(VarUint0)
{
  // TestVarUint(static_cast<uint8_t>(0));
  // TestVarUint(static_cast<uint16_t>(0));
  TestVarUint(static_cast<uint32_t>(0));
  TestVarUint(static_cast<uint64_t>(0));
}

UNIT_TEST(VarUintMinus1)
{
  // TestVarUint(static_cast<uint8_t>(-1));
  // TestVarUint(static_cast<uint16_t>(-1));
  TestVarUint(static_cast<uint32_t>(-1));
  TestVarUint(static_cast<uint64_t>(-1));
}

UNIT_TEST(VarUint32)
{
  for (int b = 0; b <= 32; ++b)
    for (uint64_t i = (1ULL << b) - 3; i <= uint32_t(-1) && i <= (1ULL << b) + 147; ++i)
      TestVarUint(static_cast<uint32_t>(i));
}

UNIT_TEST(VarInt32)
{
  for (int b = 0; b <= 32; ++b)
  {
    for (uint64_t i = (1ULL << b) - 3; i <= uint32_t(-1) && i <= (1ULL << b) + 147; ++i)
    {
      TestVarInt(static_cast<int32_t>(i));
      TestVarInt(static_cast<int32_t>(-i));
    }
  }

  int const bound = 10000;
  for (int i = -bound; i <= bound; ++i)
    TestVarInt(static_cast<int32_t>(i));

  for (int i = 0; i <= bound; ++i)
    TestVarUint(static_cast<uint32_t>(i));
}

UNIT_TEST(VarIntSize)
{
  vector<unsigned char> data;
  PushBackByteSink<vector<unsigned char>> dst(data);
  WriteVarInt(dst, 60);
  TEST_EQUAL(data.size(), 1, ());
  data.clear();
  WriteVarInt(dst, -60);
  TEST_EQUAL(data.size(), 1, ());
  data.clear();
  WriteVarInt(dst, 120);
  TEST_EQUAL(data.size(), 2, ());
  data.clear();
  WriteVarInt(dst, -120);
  TEST_EQUAL(data.size(), 2, ());
}

UNIT_TEST(VarIntMax)
{
  TestVarUint(uint32_t(-1));
  TestVarUint(uint64_t(-1));
  TestVarInt(int32_t(2147483647));
  TestVarInt(int32_t(-2147483648LL));
  TestVarInt(int64_t(9223372036854775807LL));
  // TestVarInt(int64_t(-9223372036854775808LL));
}

UNIT_TEST(ReadVarInt64Array_EmptyArray)
{
  vector<int64_t> result;
  void const * pEnd = ReadVarInt64Array(NULL, (void *)0, base::MakeBackInsertFunctor(result));
  TEST_EQUAL(result, vector<int64_t>(), ("UntilBufferEnd"));
  TEST_EQUAL(reinterpret_cast<uintptr_t>(pEnd), 0, ("UntilBufferEnd"));
  pEnd = ReadVarInt64Array(NULL, (size_t)0, base::MakeBackInsertFunctor(result));
  TEST_EQUAL(result, vector<int64_t>(), ("GivenSize"));
  TEST_EQUAL(reinterpret_cast<uintptr_t>(pEnd), 0, ("GivenSize"));
}

UNIT_TEST(ReadVarInt64Array)
{
  vector<int64_t> values;

  // Fill in values.
  {
    int64_t const baseValues[] = {0,
                                  127,
                                  128,
                                  (2 << 28) - 1,
                                  (2 << 28),
                                  (2LL << 31),
                                  (2LL << 31) - 1,
                                  0xFFFFFFFF - 1,
                                  0xFFFFFFFF,
                                  0xFFFFFFFFFFULL};
    for (size_t i = 0; i < ARRAY_SIZE(baseValues); ++i)
    {
      values.push_back(baseValues[i]);
      values.push_back(-baseValues[i]);
    }
    sort(values.begin(), values.end());
    values.erase(unique(values.begin(), values.end()), values.end());
  }

  // Test all subsets.
  for (size_t i = 1; i < 1U << values.size(); ++i)
  {
    vector<int64_t> testValues;
    for (size_t j = 0; j < values.size(); ++j)
      if (i & (1 << j))
        testValues.push_back(values[j]);

    vector<unsigned char> data;
    {
      PushBackByteSink<vector<unsigned char>> dst(data);
      for (size_t j = 0; j < testValues.size(); ++j)
        WriteVarInt(dst, testValues[j]);
    }

    ASSERT_GREATER(data.size(), 0, ());
    {
      // Factor out variables here to show the obvious compiler error.
      // clang 3.5, loop optimization.
      /// @todo Need to check with the new XCode (and clang) update.

      void const * pDataStart = &data[0];
      void const * pDataEnd = &data[0] + data.size();

      vector<int64_t> result;
      void const * pEnd = ReadVarInt64Array(pDataStart, pDataEnd, base::MakeBackInsertFunctor(result));

      TEST_EQUAL(pEnd, pDataEnd, ("UntilBufferEnd", data.size()));
      TEST_EQUAL(result, testValues, ("UntilBufferEnd", data.size()));
    }
    {
      vector<int64_t> result;
      void const * pEnd = ReadVarInt64Array(&data[0], testValues.size(), base::MakeBackInsertFunctor(result));

      TEST_EQUAL(pEnd, &data[0] + data.size(), ("GivenSize", data.size()));
      TEST_EQUAL(result, testValues, ("GivenSize", data.size()));
    }
  }
}

UNIT_TEST(VarInt_ShortSortedArray)
{
  uint32_t constexpr maxVal = (uint32_t(1) << 30) - 1;
  std::vector<uint32_t> samples[] = {
      {0},
      {10, 10000},
      {maxVal - 2, maxVal - 1, maxVal},
  };

  for (auto const & s : samples)
  {
    std::vector<uint8_t> buffer;
    PushBackByteSink sink(buffer);

    WriteVarUInt32SortedShortArray(s, sink);

    MemReader reader(buffer.data(), buffer.size());
    ReaderSource src(reader);

    std::vector<uint32_t> actual;
    ReadVarUInt32SortedShortArray(src, actual);

    TEST_EQUAL(s, actual, ());
  }
}
