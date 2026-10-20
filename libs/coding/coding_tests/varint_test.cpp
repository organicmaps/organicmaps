#include "testing/testing.hpp"

#include "coding/byte_stream.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <limits>
#include <vector>

namespace varint_test
{
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

UNIT_TEST(VarInt_ArrayByteSource_InterleavedArrays)
{
  vector<int64_t> const signedValues = {0,
                                        -1,
                                        63,
                                        -64,
                                        64,
                                        -65,
                                        int64_t{1} << 28,
                                        -(int64_t{1} << 55),
                                        numeric_limits<int64_t>::max(),
                                        numeric_limits<int64_t>::min()};
  vector<uint64_t> const unsignedValues = {
      0, 127, 128, 16383, 16384, uint64_t{1} << 28, uint64_t{1} << 56, numeric_limits<uint64_t>::max()};
  vector<int64_t> const signedTail = {-129, 128, numeric_limits<int64_t>::min()};
  uint32_t constexpr kPrimitive = 27182;
  uint8_t constexpr kSentinel = 0xab;

  vector<uint8_t> data;
  PushBackByteSink sink(data);
  for (auto const value : signedValues)
    WriteVarInt(sink, value);
  size_t const signedEnd = data.size();
  for (auto const value : unsignedValues)
    WriteVarUint(sink, value);
  size_t const unsignedEnd = data.size();
  for (auto const value : signedTail)
    WriteVarInt(sink, value);
  size_t const tailEnd = data.size();
  WriteVarUint(sink, kPrimitive);
  WriteToSink(sink, kSentinel);

  ArrayByteSource src(data.data());
  vector<int64_t> signedResult;
  ReadVarInt64Array(src, signedValues.size(), base::MakeBackInsertFunctor(signedResult));
  TEST_EQUAL(signedResult, signedValues, ());
  TEST_EQUAL(src.PtrUint8(), data.data() + signedEnd, ());

  vector<uint64_t> unsignedResult;
  ReadVarUint64Array(src, unsignedValues.size(), base::MakeBackInsertFunctor(unsignedResult));
  TEST_EQUAL(unsignedResult, unsignedValues, ());
  TEST_EQUAL(src.PtrUint8(), data.data() + unsignedEnd, ());

  signedResult.clear();
  ReadVarInt64Array(src, signedTail.size(), base::MakeBackInsertFunctor(signedResult));
  TEST_EQUAL(signedResult, signedTail, ());
  TEST_EQUAL(src.PtrUint8(), data.data() + tailEnd, ());
  TEST_EQUAL(ReadVarUint<uint32_t>(src), kPrimitive, ());
  TEST_EQUAL(src.ReadByte(), kSentinel, ());
  TEST_EQUAL(src.PtrUint8(), data.data() + data.size(), ());
}

UNIT_TEST(VarInt_ArrayByteSource_EmptyArrays)
{
  uint8_t const data[] = {0x12, 0x34};
  ArrayByteSource src(data);
  TEST_EQUAL(src.ReadByte(), data[0], ());
  size_t calls = 0;
  ReadVarInt64Array(src, 0, [&](int64_t) { ++calls; });
  ReadVarUint64Array(src, 0, [&](uint64_t) { ++calls; });
  TEST_EQUAL(calls, 0, ());
  TEST_EQUAL(src.PtrUint8(), data + 1, ());
  TEST_EQUAL(src.ReadByte(), data[1], ());

  ArrayByteSource empty(nullptr);
  ReadVarInt64Array(empty, 0, [&](int64_t) { ++calls; });
  ReadVarUint64Array(empty, 0, [&](uint64_t) { ++calls; });
  TEST_EQUAL(calls, 0, ());
  TEST_EQUAL(empty.Ptr(), nullptr, ());
}
}  // namespace varint_test
