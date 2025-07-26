#include "testing/testing.hpp"

#include "indexer/interval_index.hpp"
#include "indexer/interval_index_builder.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <utility>
#include <vector>

using namespace std;

namespace
{
struct CellIdFeaturePairForTest
{
  using ValueType = uint32_t;

  CellIdFeaturePairForTest(uint64_t cell, uint32_t value) : m_cell(cell), m_value(value) {}

  uint64_t GetCell() const { return m_cell; }
  uint32_t GetValue() const { return m_value; }

  uint64_t m_cell;
  uint32_t m_value;
};

auto IndexValueInserter(vector<uint32_t> & values)
{
  return [inserter = base::MakeBackInsertFunctor(values)](uint64_t, auto value) { inserter(value); };
}

}  // namespace

UNIT_TEST(IntervalIndex_LevelCount)
{
  TEST_EQUAL(IntervalIndexBuilder(10, 1, 3).GetLevelCount(), 1, ());
  TEST_EQUAL(IntervalIndexBuilder(11, 1, 3).GetLevelCount(), 1, ());
  TEST_EQUAL(IntervalIndexBuilder(12, 1, 3).GetLevelCount(), 2, ());
  TEST_EQUAL(IntervalIndexBuilder(19, 2, 3).GetLevelCount(), 1, ());
  TEST_EQUAL(IntervalIndexBuilder(19, 1, 3).GetLevelCount(), 4, ());
  TEST_EQUAL(IntervalIndexBuilder(20, 1, 3).GetLevelCount(), 4, ());
}

UNIT_TEST(IntervalIndex_SerializedNodeBitmap)
{
  uint32_t const offset = 350;  // == 0x15E
  uint32_t childSizes[8] = {0, 0, 0, 10, 0, 0, 1000, 0};
  char const expSerial[] =
      "\xBD\x05"  // (350 << 1) + 1 == 701 == 0x2BD - offset encoded as varuint.
      "\x48"      // (1 << 3) | (1 << 6) == 72 == 0x48 - bitmap.
      "\x0A"      // 10 - childSizes[3] encoded as varuint.
      "\xE8\x07"  // 1000 = 0x3E8 - childSizes[6] encoded as varuint.
      "";
  vector<uint8_t> serializedNode;
  MemWriter<vector<uint8_t>> writer(serializedNode);
  IntervalIndexBuilder(11, 1, 3).WriteNode(writer, offset, childSizes);
  TEST_EQUAL(serializedNode, vector<uint8_t>(expSerial, expSerial + ARRAY_SIZE(expSerial) - 1), ());
}

UNIT_TEST(IntervalIndex_SerializedNodeList)
{
  uint32_t const offset = 350;  // == 0x15E
  uint32_t childSizes[16] = {0, 0, 0, 0, 0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  char const expSerial[] =
      "\xBC\x05"  // (350 << 1) + 0 == 700 == 0x2BC - offset encoded as varuint.
      "\x06"
      "\xE8\x07"  // 6, 1000
      "";
  vector<uint8_t> serializedNode;
  MemWriter<vector<uint8_t>> writer(serializedNode);
  IntervalIndexBuilder(11, 1, 4).WriteNode(writer, offset, childSizes);
  TEST_EQUAL(serializedNode, vector<uint8_t>(expSerial, expSerial + ARRAY_SIZE(expSerial) - 1), ());
}

UNIT_TEST(IntervalIndex_SerializedLeaves)
{
  vector<CellIdFeaturePairForTest> data;
  data.push_back(CellIdFeaturePairForTest(0x1537U, 0));
  data.push_back(CellIdFeaturePairForTest(0x1538U, 1));
  data.push_back(CellIdFeaturePairForTest(0x1637U, 2));
  vector<uint8_t> serialLeaves;
  MemWriter<vector<uint8_t>> writer(serialLeaves);
  vector<uint32_t> sizes;
  IntervalIndexBuilder(16, 1, 4).BuildLeaves(writer, data.begin(), data.end(), sizes);
  char const expSerial[] =
      "\x37\x00"
      "\x38\x02"
      "\x37\x04";  // 0x1537 0x1538 0x1637
  uint32_t const expSizes[] = {4, 2};
  TEST_EQUAL(serialLeaves, vector<uint8_t>(expSerial, expSerial + ARRAY_SIZE(expSerial) - 1), ());
  TEST_EQUAL(sizes, vector<uint32_t>(expSizes, expSizes + ARRAY_SIZE(expSizes)), ());
}

UNIT_TEST(IntervalIndex_SerializedNodes)
{
  vector<CellIdFeaturePairForTest> data;
  data.push_back(CellIdFeaturePairForTest(0x1537U, 0));
  data.push_back(CellIdFeaturePairForTest(0x1538U, 1));
  data.push_back(CellIdFeaturePairForTest(0x1637U, 2));
  uint32_t const leavesSizes[] = {4, 2};
  vector<uint8_t> serialNodes;
  MemWriter<vector<uint8_t>> writer(serialNodes);
  vector<uint32_t> sizes;
  IntervalIndexBuilder(16, 1, 4).BuildLevel(writer, data.begin(), data.end(), 1, leavesSizes,
                                            leavesSizes + ARRAY_SIZE(leavesSizes), sizes);
  char const expSerial[] = "\x01\x60\x00\x04\x02";
  uint32_t const expSizes[] = {ARRAY_SIZE(expSerial) - 1};
  TEST_EQUAL(serialNodes, vector<uint8_t>(expSerial, expSerial + ARRAY_SIZE(expSerial) - 1), ());
  TEST_EQUAL(sizes, vector<uint32_t>(expSizes, expSizes + ARRAY_SIZE(expSizes)), ());
}

UNIT_TEST(IntervalIndex_Serialized)
{
  vector<CellIdFeaturePairForTest> data;
  data.push_back(CellIdFeaturePairForTest(0x1537U, 0));
  data.push_back(CellIdFeaturePairForTest(0x1538U, 1));
  data.push_back(CellIdFeaturePairForTest(0x1637U, 2));
  vector<uint8_t> serialIndex;
  MemWriter<vector<uint8_t>> writer(serialIndex);
  IntervalIndexBuilder(16, 1, 4).BuildIndex(writer, data.begin(), data.end());

  char const expSerial[] =
      "\x01\x02\x04\x01"  // Header
      "\x14\x00\x00\x00"  // Leaves level offset
      "\x1A\x00\x00\x00"  // Level 1 offset
      "\x1F\x00\x00\x00"  // Root level offset
      "\x22\x00\x00\x00"  // Root level offset
      "\x37\x00"
      "\x38\x02"
      "\x37\x04"              // 0x1537 0x1538 0x1637
      "\x01\x60\x00\x04\x02"  // 0x15, 0x16 node
      "\x00\x01\x05"          // Root
      "";

  TEST_EQUAL(serialIndex, vector<uint8_t>(expSerial, expSerial + ARRAY_SIZE(expSerial) - 1), ());

  MemReader reader(&serialIndex[0], serialIndex.size());
  IntervalIndex<MemReader, uint32_t> index(reader);
  uint32_t expected[] = {0, 1, 2};
  vector<uint32_t> values;
  TEST_EQUAL(index.KeyEnd(), 0x10000, ());
  index.ForEach(IndexValueInserter(values), 0, 0x10000);
  TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
}

UNIT_TEST(IntervalIndex_Simple)
{
  vector<CellIdFeaturePairForTest> data;
  data.push_back(CellIdFeaturePairForTest(0xA0B1C2D100ULL, 0));
  data.push_back(CellIdFeaturePairForTest(0xA0B1C2D200ULL, 1));
  data.push_back(CellIdFeaturePairForTest(0xA0B2C2D100ULL, 2));
  vector<char> serialIndex;
  MemWriter<vector<char>> writer(serialIndex);
  BuildIntervalIndex(data.begin(), data.end(), writer, 40);
  MemReader reader(&serialIndex[0], serialIndex.size());
  IntervalIndex<MemReader, uint32_t> index(reader);
  TEST_EQUAL(index.KeyEnd(), 0x10000000000ULL, ());
  {
    uint32_t expected[] = {0, 1, 2};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0ULL, index.KeyEnd());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    uint32_t expected[] = {0, 1};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0xA0B1C2D100ULL, 0xA0B1C2D201ULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    uint32_t expected[] = {0, 1};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0x0ULL, 0xA0B1C30000ULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    uint32_t expected[] = {0};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0xA0B1C2D100ULL, 0xA0B1C2D101ULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    uint32_t expected[] = {0};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0xA0B1C2D100ULL, 0xA0B1C2D200ULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0xA0B1C2D100ULL, 0xA0B1C2D100ULL);
    TEST_EQUAL(values, vector<uint32_t>(), ());
  }
  {
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0xA0B1000000ULL, 0xA0B1B20000ULL);
    TEST_EQUAL(values, vector<uint32_t>(), ());
  }
}

UNIT_TEST(IntervalIndex_Empty)
{
  vector<CellIdFeaturePairForTest> data;
  vector<char> serialIndex;
  MemWriter<vector<char>> writer(serialIndex);
  BuildIntervalIndex(data.begin(), data.end(), writer, 40);
  MemReader reader(&serialIndex[0], serialIndex.size());
  IntervalIndex<MemReader, uint32_t> index(reader);
  {
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0ULL, 0xFFFFFFFFFFULL);
    TEST_EQUAL(values, vector<uint32_t>(), ());
  }
}

UNIT_TEST(IntervalIndex_Simple2)
{
  vector<CellIdFeaturePairForTest> data;
  data.push_back(CellIdFeaturePairForTest(0xA0B1C2D200ULL, 0));
  data.push_back(CellIdFeaturePairForTest(0xA0B1C2D200ULL, 1));
  data.push_back(CellIdFeaturePairForTest(0xA0B1C2D200ULL, 3));
  data.push_back(CellIdFeaturePairForTest(0xA0B2C2D200ULL, 2));
  vector<char> serialIndex;
  MemWriter<vector<char>> writer(serialIndex);
  BuildIntervalIndex(data.begin(), data.end(), writer, 40);
  MemReader reader(&serialIndex[0], serialIndex.size());
  IntervalIndex<MemReader, uint32_t> index(reader);
  {
    uint32_t expected[] = {0, 1, 2, 3};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0, 0xFFFFFFFFFFULL);
    sort(values.begin(), values.end());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
}

UNIT_TEST(IntervalIndex_Simple3)
{
  vector<CellIdFeaturePairForTest> data;
  data.push_back(CellIdFeaturePairForTest(0x0100ULL, 0));
  data.push_back(CellIdFeaturePairForTest(0x0200ULL, 1));
  vector<char> serialIndex;
  MemWriter<vector<char>> writer(serialIndex);
  BuildIntervalIndex(data.begin(), data.end(), writer, 40);
  MemReader reader(&serialIndex[0], serialIndex.size());
  IntervalIndex<MemReader, uint32_t> index(reader);
  {
    uint32_t expected[] = {0, 1};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0, 0xFFFFULL);
    sort(values.begin(), values.end());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
}

UNIT_TEST(IntervalIndex_Simple4)
{
  vector<CellIdFeaturePairForTest> data;
  data.push_back(CellIdFeaturePairForTest(0x01030400ULL, 0));
  data.push_back(CellIdFeaturePairForTest(0x02030400ULL, 1));
  vector<char> serialIndex;
  MemWriter<vector<char>> writer(serialIndex);
  BuildIntervalIndex(data.begin(), data.end(), writer, 40);
  MemReader reader(&serialIndex[0], serialIndex.size());
  IntervalIndex<MemReader, uint32_t> index(reader);
  {
    uint32_t expected[] = {0, 1};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0, 0xFFFFFFFFULL);
    sort(values.begin(), values.end());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
}

UNIT_TEST(IntervalIndex_Simple5)
{
  vector<CellIdFeaturePairForTest> data;
  data.push_back(CellIdFeaturePairForTest(0xA0B1C2D200ULL, 0));
  data.push_back(CellIdFeaturePairForTest(0xA0B1C2D200ULL, 1));
  data.push_back(CellIdFeaturePairForTest(0xA0B1C2D200ULL, 3));
  data.push_back(CellIdFeaturePairForTest(0xA0B2C2D200ULL, 2));
  vector<char> serialIndex;
  MemWriter<vector<char>> writer(serialIndex);
  BuildIntervalIndex(data.begin(), data.end(), writer, 40);
  MemReader reader(&serialIndex[0], serialIndex.size());
  IntervalIndex<MemReader, uint32_t> index(reader);
  {
    uint32_t expected[] = {0, 1, 2, 3};
    vector<uint32_t> values;
    index.ForEach(IndexValueInserter(values), 0, 0xFFFFFFFFFFULL);
    sort(values.begin(), values.end());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
}
