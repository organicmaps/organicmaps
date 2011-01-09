#include "../varint.hpp"
#include "../../testing/testing.hpp"

#include "../byte_stream.hpp"
#include "../../base/macros.hpp"
#include "../../base/stl_add.hpp"

namespace
{
  template <typename T> void TestVarUint(T const x)
  {
    vector<unsigned char> data;
    PushBackByteSink<vector<unsigned char> > dst(data);
    WriteVarUint(dst, x);
    ArrayByteSource src(&data[0]);
    TEST_EQUAL(ReadVarUint<T>(src), x, ());
    size_t const bytesRead = static_cast<const unsigned char*>(src.Ptr()) - &data[0];
    TEST_EQUAL(bytesRead, data.size(), (x));
  }

  template <typename T> void TestVarInt(T const x)
  {
    vector<unsigned char> data;
    PushBackByteSink<vector<unsigned char> > dst(data);
    WriteVarInt(dst, x);
    ArrayByteSource src(&data[0]);
    TEST_EQUAL(ReadVarInt<T>(src), x, ());
    size_t const bytesRead = static_cast<const unsigned char*>(src.Ptr()) - &data[0];
    TEST_EQUAL(bytesRead, data.size(), (x));
  }
}

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
  for (int i = -300; i <= 300; ++i)
  {
    TestVarInt(static_cast<int32_t>(i));
    TestVarInt(static_cast<int32_t>(-i));
  }
}

UNIT_TEST(VarIntSize)
{
  vector<unsigned char> data;
  PushBackByteSink<vector<unsigned char> > dst(data);
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
  TestVarInt(int32_t(-2147483648));
  TestVarInt(int64_t(9223372036854775807LL));
  // TestVarInt(int64_t(-9223372036854775808LL));
}

UNIT_TEST(ReadVarInt64Array_EmptyArray)
{
  vector<int64_t> result;
  ReadVarInt64Array(NULL, NULL, MakeBackInsertFunctor(result));
  TEST_EQUAL(result, vector<int64_t>(), ());
}

UNIT_TEST(ReadVarInt64Array)
{
  vector<int64_t> values;

  // Fill in values.
  {
    int64_t const baseValues [] =
    {
      0, 127, 128, (2 << 28) - 1, (2 << 28), (2 << 31), (2 << 31) - 1,
      0xFFFFFFFF - 1, 0xFFFFFFFF, 0xFFFFFFFFFFULL
    };
    for (size_t i = 0; i < ARRAY_SIZE(baseValues); ++i)
    {
      values.push_back(baseValues[i]);
      values.push_back(-baseValues[i]);
    }
    sort(values.begin(), values.end());
    values.erase(unique(values.begin(), values.end()), values.end());
  }

  // Test all subsets.
  for (size_t i = 1; i < 1 << values.size(); ++i)
  {
    vector<int64_t> testValues;
    for (size_t j = 0; j < values.size(); ++j)
      if (i & (1 << j))
        testValues.push_back(values[j]);

    vector<unsigned char> data;
    {
      PushBackByteSink<vector<unsigned char> > dst(data);
      for (size_t j = 0; j < testValues.size(); ++j)
        WriteVarInt(dst, testValues[j]);
    }

    vector<int64_t> result;
    ASSERT_GREATER(data.size(), 0, ());
    ReadVarInt64Array(&data[0], &data[0] + data.size(), MakeBackInsertFunctor(result));
    TEST_EQUAL(result, testValues, ());
  }
}

