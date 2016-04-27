#include "testing/testing.hpp"

#include "base/mem_trie.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

UNIT_TEST(MemTrie_Basic)
{
  vector<pair<string, int>> data = {{"roger", 3},
                                    {"amy", 1},
                                    {"emma", 1},
                                    {"ann", 1},
                                    {"rob", 1},
                                    {"roger", 2},
                                    {"", 0},
                                    {"roger", 1}};
  my::MemTrie<string, int> trie;
  for (auto const & p : data)
    trie.Add(p.first, p.second);

  vector<pair<string, int>> trie_data;
  trie.ForEach([&trie_data](string const & k, int v)
               {
                 trie_data.emplace_back(k, v);
               });
  sort(data.begin(), data.end());
  sort(trie_data.begin(), trie_data.end());
  TEST_EQUAL(data, trie_data, ());
}
