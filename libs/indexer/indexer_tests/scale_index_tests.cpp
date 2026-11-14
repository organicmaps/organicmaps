#include "testing/testing.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/cell_value_pair.hpp"
#include "indexer/interval_index_builder.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/v2/cell_trie.hpp"

#include "coding/reader.hpp"
#include "coding/var_serial_vector.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <tuple>
#include <vector>

namespace scale_index_tests
{
using Version = ScaleIndexBase::Version;
using Bytes = std::vector<uint8_t>;
using Ids = std::vector<uint32_t>;
// A bucket, a cell number and a feature.
using Item = std::tuple<uint32_t, uint64_t, uint32_t>;

int constexpr kDepth = RectId::DEPTH_LEVELS;
uint32_t constexpr kBuckets = 18;

// The cells of a map: of the last level for the points and bigger ones for the other features.
std::vector<Item> MakeItems(std::mt19937 & rng, size_t count)
{
  std::vector<Item> items;
  uint64_t const area = rng();
  for (size_t i = 0; i < count; ++i)
  {
    int const level = (rng() % 3 == 0) ? kDepth - 1 : 9 + rng() % 7;
    uint64_t bits = ((area << 12) | (rng() & 0xFFF)) & ((uint64_t{1} << 2 * (kDepth - 1)) - 1);
    bits >>= 2 * (kDepth - 1 - level);
    auto const cell = static_cast<uint64_t>(RectId::FromBitsAndLevel(bits, level).ToInt64(kDepth));
    items.emplace_back(rng() % kBuckets, cell, static_cast<uint32_t>(i / 2 + rng() % 16));
  }
  std::sort(items.begin(), items.end());
  items.erase(std::unique(items.begin(), items.end()), items.end());
  return items;
}

// The index of the version 1, as the maps before the cells trie have it.
Bytes BuildV1(std::vector<Item> const & items)
{
  Bytes bytes;
  MemWriter writer(bytes);
  VarSerialVectorWriter recordWriter(writer, kBuckets);
  auto it = items.begin();
  for (uint32_t bucket = 0; bucket < kBuckets; ++bucket)
  {
    std::vector<covering::CellValuePair<uint32_t>> cellsToFeatures;
    for (; it != items.end() && std::get<0>(*it) == bucket; ++it)
      cellsToFeatures.emplace_back(std::get<1>(*it), std::get<2>(*it));

    SubWriter subWriter(writer);
    BuildIntervalIndex(cellsToFeatures.begin(), cellsToFeatures.end(), subWriter, RectId::DEPTH_LEVELS * 2 + 1);
    recordWriter.FinishRecord();
  }
  return bytes;
}

// The index of the latest version, as covering::IndexScales() writes it.
Bytes BuildV2(std::vector<Item> const & items)
{
  Bytes bytes;
  MemWriter writer(bytes);
  ScaleIndexBase::Header const header;
  writer.Write(&header, sizeof(header));
  idx::v2::CellTrieBuilder builder(kDepth, kBuckets);
  for (auto const & [bucket, cell, feature] : items)
    builder.Add(bucket, cell, feature);
  builder.Flush(writer);
  return bytes;
}

Ids Find(ScaleIndex<MemReader> const & index, uint64_t beg, uint64_t end, int scale)
{
  Ids ids;
  index.ForEachInIntervalAndScale(beg, end, scale, [&ids](uint32_t feature) { ids.push_back(feature); });
  std::sort(ids.begin(), ids.end());
  return ids;
}

// The reader finds the same features in the index of any version.
UNIT_TEST(ScaleIndex_Versions)
{
  std::mt19937 rng(11);
  auto const items = MakeItems(rng, 20000);

  auto const bytesV1 = BuildV1(items);
  auto const bytesV2 = BuildV2(items);

  ScaleIndex<MemReader> const indexV1(MemReader(bytesV1.data(), bytesV1.size()));
  ScaleIndex<MemReader> const indexV2(MemReader(bytesV2.data(), bytesV2.size()));
  TEST(indexV1.GetVersion() == Version::V1, ());
  TEST(indexV2.GetVersion() == Version::Latest, ());

  size_t found = 0;
  auto const check = [&](uint64_t beg, uint64_t end, int scale)
  {
    Ids expected;
    for (auto const & [bucket, cell, feature] : items)
      if (static_cast<int>(bucket) <= scale && scale < static_cast<int>(kBuckets) && cell >= beg && cell < end)
        expected.push_back(feature);
    std::sort(expected.begin(), expected.end());
    found += expected.size();

    TEST_EQUAL(Find(indexV1, beg, end, scale), expected, (beg, end, scale));
    TEST_EQUAL(Find(indexV2, beg, end, scale), expected, (beg, end, scale));
  };

  uint64_t const treeSize = RectId::Root().SubTreeSize(kDepth);
  for (int scale = 0; scale <= static_cast<int>(kBuckets) + 1; ++scale)
    check(0, treeSize + 1, scale);

  for (int i = 0; i < 300; ++i)
  {
    uint64_t const number = std::get<1>(items[rng() % items.size()]);
    auto cell = RectId::FromInt64(static_cast<int64_t>(number), kDepth);
    for (int up = rng() % 6; up > 0 && cell.Level() > 0; --up)
      cell = cell.Parent();
    uint64_t const beg = static_cast<uint64_t>(cell.ToInt64(kDepth));
    int const scale = rng() % (kBuckets + 2);

    switch (rng() % 3)
    {
    case 0: check(beg, beg + cell.SubTreeSize(kDepth), scale); break;
    case 1: check(beg, beg + 1, scale); break;
    default: check(beg, number + 1 + rng() % 300, scale);
    }
  }
  TEST_GREATER(found, items.size(), ());
}
}  // namespace scale_index_tests
