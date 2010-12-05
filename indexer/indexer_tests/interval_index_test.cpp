#include "../../testing/testing.hpp"
#include "../interval_index.hpp"
#include "../interval_index_builder.hpp"
#include "../../coding/reader.hpp"
#include "../../coding/writer.hpp"
#include "../../base/macros.hpp"
#include "../../base/stl_add.hpp"
#include "../../std/utility.hpp"
#include "../../std/vector.hpp"

UNIT_TEST(IntervalIndex_Simple)
{
  vector<pair<int64_t, uint32_t> > data;
  data.push_back(make_pair(0xA0B1C2D100ULL, 0));
  data.push_back(make_pair(0xA0B1C2D200ULL, 1));
  data.push_back(make_pair(0xA0B2C2D100ULL, 2));
  vector<char> serializedIndex;
  MemWriter<vector<char> > writer(serializedIndex);
  BuildIntervalIndex<5>(data.begin(), data.end(), writer, 2);
  MemReader reader(&serializedIndex[0], serializedIndex.size());
  IntervalIndex<uint32_t, MemReader> index(reader, 5);
  {
    uint32_t expected [] = {0, 1, 2};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0ULL, 0xFFFFFFFFFFULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    uint32_t expected [] = {0, 1};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0xA0B1C2D100ULL, 0xA0B1C2D201ULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    uint32_t expected [] = {0, 1};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0x0ULL, 0xA0B1C30000ULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    uint32_t expected [] = {0};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0xA0B1C2D100ULL, 0xA0B1C2D101ULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    uint32_t expected [] = {0};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0xA0B1C2D100ULL, 0xA0B1C2D200ULL);
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
  {
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0xA0B1C2D100ULL, 0xA0B1C2D100ULL);
    TEST_EQUAL(values, vector<uint32_t>(), ());
  }
  {
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0xA0B1000000ULL, 0xA0B1B20000ULL);
    TEST_EQUAL(values, vector<uint32_t>(), ());
  }
}

UNIT_TEST(IntervalIndex_Empty)
{
  vector<pair<int64_t, uint32_t> > data;
  vector<char> serializedIndex;
  MemWriter<vector<char> > writer(serializedIndex);
  BuildIntervalIndex<5>(data.begin(), data.end(), writer, 2);
  MemReader reader(&serializedIndex[0], serializedIndex.size());
  IntervalIndex<uint32_t, MemReader> index(reader, 5);
  {
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0ULL, 0xFFFFFFFFFFULL);
    TEST_EQUAL(values, vector<uint32_t>(), ());
  }
}

UNIT_TEST(IntervalIndex_Simple2)
{
  vector<pair<int64_t, uint32_t> > data;
  data.push_back(make_pair(0xA0B1C2D200ULL, 0));
  data.push_back(make_pair(0xA0B1C2D200ULL, 1));
  data.push_back(make_pair(0xA0B1C2D200ULL, 3));
  data.push_back(make_pair(0xA0B2C2D200ULL, 2));
  vector<char> serializedIndex;
  MemWriter<vector<char> > writer(serializedIndex);
  BuildIntervalIndex<5>(data.begin(), data.end(), writer, 2);
  MemReader reader(&serializedIndex[0], serializedIndex.size());
  IntervalIndex<uint32_t, MemReader> index(reader, 5);
  {
    uint32_t expected [] = {0, 1, 2, 3};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0, 0xFFFFFFFFFFULL);
    sort(values.begin(), values.end());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
}

UNIT_TEST(IntervalIndex_Simple3)
{
  vector<pair<uint64_t, uint32_t> > data;
  data.push_back(make_pair(0x0100ULL, 0));
  data.push_back(make_pair(0x0200ULL, 1));
  vector<char> serializedIndex;
  MemWriter<vector<char> > writer(serializedIndex);
  BuildIntervalIndex<2>(data.begin(), data.end(), writer, 1);
  MemReader reader(&serializedIndex[0], serializedIndex.size());
  IntervalIndex<uint32_t, MemReader> index(reader, 2);
  {
    uint32_t expected [] = {0, 1};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0, 0xFFFFULL);
    sort(values.begin(), values.end());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
}

UNIT_TEST(IntervalIndex_Simple4)
{
  vector<pair<uint64_t, uint32_t> > data;
  data.push_back(make_pair(0x01030400ULL, 0));
  data.push_back(make_pair(0x02030400ULL, 1));
  vector<char> serializedIndex;
  MemWriter<vector<char> > writer(serializedIndex);
  BuildIntervalIndex<4>(data.begin(), data.end(), writer, 1);
  MemReader reader(&serializedIndex[0], serializedIndex.size());
  IntervalIndex<uint32_t, MemReader> index(reader, 4);
  {
    uint32_t expected [] = {0, 1};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0, 0xFFFFFFFFULL);
    sort(values.begin(), values.end());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
}

UNIT_TEST(IntervalIndex_Simple5)
{
  vector<pair<uint64_t, uint32_t> > data;
  data.push_back(make_pair(0xA0B1C2D200ULL, 0));
  data.push_back(make_pair(0xA0B1C2D200ULL, 1));
  data.push_back(make_pair(0xA0B1C2D200ULL, 3));
  data.push_back(make_pair(0xA0B2C2D200ULL, 2));
  vector<char> serializedIndex;
  MemWriter<vector<char> > writer(serializedIndex);
  BuildIntervalIndex<5>(data.begin(), data.end(), writer, 1);
  MemReader reader(&serializedIndex[0], serializedIndex.size());
  IntervalIndex<uint32_t, MemReader> index(reader, 5);
  {
    uint32_t expected [] = {0, 1, 2, 3};
    vector<uint32_t> values;
    index.ForEach(MakeBackInsertFunctor(values), 0, 0xFFFFFFFFFFULL);
    sort(values.begin(), values.end());
    TEST_EQUAL(values, vector<uint32_t>(expected, expected + ARRAY_SIZE(expected)), ());
  }
}
