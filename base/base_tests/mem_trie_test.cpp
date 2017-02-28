#include "testing/testing.hpp"

#include "base/mem_trie.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace
{
using Trie = my::MemTrie<string, my::VectorValues<int>>;
using Data = std::vector<std::pair<std::string, int>>;

void GetTrieContents(Trie const & trie, Data & data)
{
  data.clear();
  trie.ForEachInTrie([&data](std::string const & k, int v) { data.emplace_back(k, v); });
  std::sort(data.begin(), data.end());
}

UNIT_TEST(MemTrie_Basic)
{
  Data data = {{"roger", 3}, {"amy", 1},   {"emma", 1}, {"ann", 1},
               {"rob", 1},   {"roger", 2}, {"", 0},     {"roger", 1}};
  Trie trie;
  TEST_EQUAL(trie.GetNumNodes(), 1, ());

  for (auto const & p : data)
    trie.Add(p.first, p.second);
  TEST_EQUAL(trie.GetNumNodes(), 16, ());

  std::sort(data.begin(), data.end());

  Data contents;
  GetTrieContents(trie, contents);
  TEST_EQUAL(contents, data, ());

  TEST_EQUAL(trie.GetNumNodes(), 16, ());

  Trie newTrie(move(trie));

  TEST_EQUAL(trie.GetNumNodes(), 1, ());
  GetTrieContents(trie, contents);
  TEST(contents.empty(), ());

  TEST_EQUAL(newTrie.GetNumNodes(), 16, ());
  GetTrieContents(newTrie, contents);
  TEST_EQUAL(contents, data, ());
}
}  // namespace
