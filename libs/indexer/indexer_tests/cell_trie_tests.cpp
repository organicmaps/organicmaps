#include "testing/testing.hpp"

#include "indexer/indexer_tests/counting_reader.hpp"
#include "indexer/v2/cell_trie.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/cellid.hpp"

#include "platform/platform.hpp"

#include "base/scope_guard.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <tuple>
#include <vector>

namespace cell_trie_tests
{
using namespace idx::v2;

// The cells of the geometry index of a map.
using CellId = m2::CellId<19>;
using Ids = std::vector<uint32_t>;
// A root, a cell number and an id.
using Item = std::tuple<uint32_t, uint64_t, uint32_t>;

std::vector<uint8_t> Build(std::vector<Item> const & items, uint8_t depth, uint8_t roots, uint32_t blockIds)
{
  CellTrieBuilder builder(depth, roots, blockIds);
  for (auto const & [root, cell, id] : items)
    builder.Add(root, cell, id);
  std::vector<uint8_t> bytes;
  MemWriter writer(bytes);
  builder.Flush(writer);
  return bytes;
}

Ids Expected(std::vector<Item> const & items, uint64_t beg, uint64_t end, uint32_t rootsEnd)
{
  Ids ids;
  for (auto const & [root, cell, id] : items)
    if (root < rootsEnd && cell >= beg && cell < end)
      ids.push_back(id);
  std::sort(ids.begin(), ids.end());
  return ids;
}

template <class TCursor>
Ids Found(TCursor & cursor, uint64_t beg, uint64_t end, uint32_t rootsEnd)
{
  Ids ids;
  cursor.ForEach(beg, end, rootsEnd, [&ids](uint32_t id) { ids.push_back(id); });
  std::sort(ids.begin(), ids.end());
  return ids;
}

UNIT_TEST(CellTrie_Smoke)
{
  int constexpr kDepth = 19;
  auto const number = [](std::string const & cell)
  { return static_cast<uint64_t>(CellId::FromString(cell).ToInt64(kDepth)); };
  std::string const leaf = "012301230123012301";
  TEST_EQUAL(leaf.size(), kDepth - 1, ());

  std::vector<Item> const items = {
      {0, number(""), 7},     {0, number("0"), 1},  {0, number("01"), 2},    {0, number("01"), 3},
      {0, number("0123"), 4}, {0, number(leaf), 5}, {0, number(leaf), 4},    {1, number("3"), 6},
      {1, number("01"), 8},   {2, number("2"), 9},  {2, number("0123"), 10},
  };

  // A block per token, then blocks of several tokens.
  for (uint32_t const blockIds : {0U, 1000U})
  {
    auto const bytes = Build(items, kDepth, 4 /* roots */, blockIds);
    MemReader const reader(bytes.data(), bytes.size());
    CellTrie<MemReader> const trie(reader);
    TEST_EQUAL(trie.GetHeader().m_tokens, 9, ());
    CellTrie<MemReader>::Cursor cursor(trie);

    auto const subtree = [&](std::string const & cell, uint32_t rootsEnd)
    {
      uint64_t const beg = number(cell);
      return Found(cursor, beg, beg + CellId::FromString(cell).SubTreeSize(kDepth), rootsEnd);
    };
    auto const cell = [&](std::string const & cell, uint32_t rootsEnd)
    { return Found(cursor, number(cell), number(cell) + 1, rootsEnd); };

    TEST_EQUAL(subtree("", 1), (Ids{1, 2, 3, 4, 4, 5, 7}), ());
    TEST_EQUAL(subtree("", 4), (Ids{1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10}), ());
    TEST_EQUAL(subtree("", 100), (Ids{1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10}), ());
    TEST_EQUAL(subtree("", 0), Ids(), ());
    TEST_EQUAL(subtree("0", 1), (Ids{1, 2, 3, 4, 4, 5}), ());
    TEST_EQUAL(subtree("01", 2), (Ids{2, 3, 4, 4, 5, 8}), ());
    TEST_EQUAL(subtree("012", 3), (Ids{4, 4, 5, 10}), ());
    TEST_EQUAL(subtree("0123", 1), (Ids{4, 4, 5}), ());
    TEST_EQUAL(subtree("01230", 3), (Ids{4, 5}), ());
    TEST_EQUAL(subtree(leaf, 3), (Ids{4, 5}), ());
    TEST_EQUAL(subtree("1", 4), Ids(), ());
    TEST_EQUAL(subtree("3", 4), (Ids{6}), ());
    TEST_EQUAL(subtree("0122", 4), Ids(), ());

    TEST_EQUAL(cell("", 4), (Ids{7}), ());
    TEST_EQUAL(cell("0", 4), (Ids{1}), ());
    TEST_EQUAL(cell("01", 1), (Ids{2, 3}), ());
    TEST_EQUAL(cell("01", 4), (Ids{2, 3, 8}), ());
    TEST_EQUAL(cell("012", 4), Ids(), ());
    TEST_EQUAL(cell(leaf, 4), (Ids{4, 5}), ());

    // From a cell to the middle of another subtree, and beyond the tree.
    TEST_EQUAL(Found(cursor, number("01"), number("0123") + 1, 1), (Ids{2, 3, 4}), ());
    TEST_EQUAL(Found(cursor, number("0123") + 1, number("3"), 4), (Ids{4, 5, 9}), ());
    TEST_EQUAL(Found(cursor, 0, uint64_t{1} << 62, 4), (Ids{1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10}), ());
    TEST_EQUAL(Found(cursor, 5, 5, 4), Ids(), ());
  }
}

UNIT_TEST(CellTrie_Empty)
{
  auto const bytes = Build({}, 19 /* depth */, 18 /* roots */, CellTrieBase::kBlockIds);
  MemReader const reader(bytes.data(), bytes.size());
  CellTrie<MemReader> const trie(reader);
  TEST_EQUAL(trie.GetHeader().m_tokens, 0, ());
  TEST_EQUAL(trie.GetNodesCount(), 18, ());
  TEST_EQUAL(trie.GetPacketsPos(), bytes.size(), ());
  CellTrie<MemReader>::Cursor cursor(trie);
  TEST_EQUAL(Found(cursor, 0, uint64_t{1} << 62, 18), Ids(), ());
  TEST_EQUAL(Found(cursor, 12345, 12346, 18), Ids(), ());
}

// Clustered cells of the levels that a map has, and a few random ones.
std::vector<Item> MakeItems(std::mt19937 & rng, int depth, uint32_t roots, size_t count)
{
  std::vector<Item> items;
  uint64_t const area = rng();
  for (size_t i = 0; i < count; ++i)
  {
    int level = depth - 1;
    if (rng() % 3 == 0)
      level = std::max(0, depth - 4 - static_cast<int>(rng() % 4));
    if (rng() % 50 == 0)
      level = rng() % depth;

    // A cell of the area: twelve random low bits under the common high ones.
    uint64_t bits = ((area << 12) | (rng() & 0xFFF)) & ((uint64_t{1} << 2 * (depth - 1)) - 1);
    if (rng() % 20 == 0)
      bits = (uint64_t{rng()} << 32 | rng()) & ((uint64_t{1} << 2 * (depth - 1)) - 1);
    bits >>= 2 * (depth - 1 - level);

    auto const number = static_cast<uint64_t>(CellId::FromBitsAndLevel(bits, level).ToInt64(depth));
    uint32_t const id = (rng() % 4 == 0) ? rng() : static_cast<uint32_t>(i / 3 + rng() % 8);
    items.emplace_back(rng() % roots, number, id);
  }
  std::sort(items.begin(), items.end());
  items.erase(std::unique(items.begin(), items.end()), items.end());
  return items;
}

// Compares the lookups of the subtrees, of the cells and of other intervals with the items.
template <class TCursor>
void CheckLookups(std::mt19937 & rng, std::vector<Item> const & items, TCursor & cursor, int depth, uint32_t roots)
{
  uint64_t const treeSize = CellId::Root().SubTreeSize(depth);
  auto const check = [&](uint64_t beg, uint64_t end)
  {
    uint32_t const rootsEnd = 1 + rng() % roots;
    TEST_EQUAL(Found(cursor, beg, end, rootsEnd), Expected(items, beg, end, rootsEnd), (beg, end, rootsEnd, depth));
  };

  check(0, treeSize + 100);
  check(1, treeSize + 1);
  for (int i = 0; i < 400; ++i)
  {
    uint64_t const number = std::get<1>(items[rng() % items.size()]);
    auto cell = CellId::FromInt64(static_cast<int64_t>(number), depth);
    for (int up = rng() % 6; up > 0 && cell.Level() > 0; --up)
      cell = cell.Parent();
    uint64_t const beg = static_cast<uint64_t>(cell.ToInt64(depth));

    switch (rng() % 5)
    {
    case 0: check(beg, beg + cell.SubTreeSize(depth)); break;
    case 1: check(beg, beg + 1); break;
    case 2: check(beg, std::min(treeSize + 1, beg + 1 + rng() % 5000)); break;
    case 3: check(beg - std::min(beg, uint64_t{rng() % 300}), number + rng() % 300); break;
    default:
    {
      uint64_t const other = std::get<1>(items[rng() % items.size()]);
      check(std::min(number, other), std::max(number, other) + rng() % 2);
    }
    }
  }
}

UNIT_TEST(CellTrie_Random)
{
  std::string const path = GetPlatform().TmpPathForFile("cell_trie_test", ".trie");
  SCOPE_GUARD(removeFile, [&path] { FileWriter::DeleteFileX(path); });

  std::mt19937 rng(42);
  for (int const depth : {19, 11, 3})
  {
    for (uint32_t const roots : {1U, 18U})
    {
      auto const items = MakeItems(rng, depth, roots, 4000);
      for (uint32_t const blockIds : {0U, 8U, 1000U})
      {
        auto const bytes = Build(items, depth, roots, blockIds);
        {
          // A reader of memory.
          MemReader const reader(bytes.data(), bytes.size());
          CellTrie<MemReader> const trie(reader);
          CellTrie<MemReader>::Cursor cursor(trie);
          CheckLookups(rng, items, cursor, depth, roots);
        }
        {
          // A reader of a file, where the trie follows other data.
          size_t constexpr kPrefix = 13;
          {
            FileWriter writer(path);
            writer.Write(std::string(kPrefix, 'x').data(), kPrefix);
            writer.Write(bytes.data(), bytes.size());
          }
          FileReader const file(path);
          FileReader const reader = file.SubReader(kPrefix, bytes.size());
          CellTrie<FileReader> const trie(reader);
          CellTrie<FileReader>::Cursor cursor(trie);
          CheckLookups(rng, items, cursor, depth, roots);
        }
      }
    }
  }
}

// Nothing is loaded: a trie is opened by reading its header and levels, a lookup reads the
// words of the vectors and the packets that it needs. A reader that is not a reader of memory
// is called less: its vectors read by windows.
UNIT_TEST(CellTrie_ReadsOnDemand)
{
  using MemoryReader = indexer_tests::CountingReader<true /* IsMemoryReader */>;
  using FileLikeReader = indexer_tests::CountingReader<false /* IsMemoryReader */>;

  int constexpr kDepth = 19;
  uint32_t constexpr kRoots = 18;
  std::mt19937 rng(5);
  auto const items = MakeItems(rng, kDepth, kRoots, 40000);
  auto const bytes = Build(items, kDepth, kRoots, CellTrieBase::kBlockIds);
  TEST_GREATER(bytes.size(), 100000, ());

  MemoryReader const memoryReader(bytes);
  CellTrie<MemoryReader> const memoryTrie(memoryReader);
  CellTrie<MemoryReader>::Cursor memoryCursor(memoryTrie);
  TEST_LESS(memoryReader.GetReadBytes(), 1024, ());

  FileLikeReader const fileReader(bytes);
  CellTrie<FileLikeReader> const fileTrie(fileReader);
  CellTrie<FileLikeReader>::Cursor fileCursor(fileTrie);
  TEST_LESS(fileReader.GetReadBytes(), 2048, ());

  // A cell of the last level.
  memoryReader.ResetCounters();
  uint64_t const number = std::get<1>(items.back());
  TEST_EQUAL(Found(memoryCursor, number, number + 1, kRoots), Expected(items, number, number + 1, kRoots), ());
  TEST_LESS(memoryReader.GetReadBytes(), 8192, ());

  // The same lookups through both readers.
  memoryReader.ResetCounters();
  fileReader.ResetCounters();
  std::mt19937 memoryRng(6);
  std::mt19937 fileRng(6);
  CheckLookups(memoryRng, items, memoryCursor, kDepth, kRoots);
  CheckLookups(fileRng, items, fileCursor, kDepth, kRoots);
  TEST_LESS(2 * fileReader.GetReads(), memoryReader.GetReads(), ());
}
}  // namespace cell_trie_tests
