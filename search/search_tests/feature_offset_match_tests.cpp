#include "testing/testing.hpp"

#include "search/feature_offset_match.hpp"

#include "indexer/trie.hpp"

#include "base/mem_trie.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace base;
using namespace std;

namespace
{
using Key = strings::UniString;
using Value = uint32_t;
using ValueList = VectorValues<Value>;
using Trie = MemTrie<Key, ValueList>;

UNIT_TEST(MatchInTrieTest)
{
  Trie trie;

  vector<pair<string, uint32_t>> const data = {{"hotel", 1}, {"homel", 2}, {"hotel", 3}};

  for (auto const & kv : data)
    trie.Add(strings::MakeUniString(kv.first), kv.second);

  trie::MemTrieIterator<Key, ValueList> const rootIterator(trie.GetRootIterator());
  map<uint32_t, bool> vals;
  auto saveResult = [&vals](uint32_t v, bool exactMatch) { vals[v] = exactMatch; };

  auto const hotelDFA = strings::LevenshteinDFA("hotel", 1 /* maxErrors */);
  search::impl::MatchInTrie(rootIterator, nullptr, 0 /* prefixSize */, hotelDFA, saveResult);
  TEST(vals.at(1), (vals));
  TEST(vals.at(3), (vals));
  TEST(!vals.at(2), (vals));

  vals.clear();
  auto const homelDFA = strings::LevenshteinDFA("homel", 1 /* maxErrors */);
  search::impl::MatchInTrie(rootIterator, nullptr, 0 /* prefixSize */, homelDFA, saveResult);
  TEST(vals.at(2), (vals));
  TEST(!vals.at(1), (vals));
  TEST(!vals.at(3), (vals));
}
}  // namespace
