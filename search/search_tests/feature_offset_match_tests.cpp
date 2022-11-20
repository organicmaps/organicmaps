#include "testing/testing.hpp"

#include "search/feature_offset_match.hpp"

#include "indexer/trie.hpp"

#include "base/dfa_helpers.hpp"
#include "base/mem_trie.hpp"
#include "base/string_utils.hpp"

#include <map>
#include <string>
#include <vector>

namespace feature_offset_match_tests
{
using namespace base;
using namespace std;

using Key = strings::UniString;
using Value = uint32_t;
using ValueList = VectorValues<Value>;
using Trie = MemTrie<Key, ValueList>;
using DFA = strings::LevenshteinDFA;
using PrefixDFA = strings::PrefixDFAModifier<DFA>;

UNIT_TEST(MatchInTrieTest)
{
  Trie trie;

  vector<pair<string, uint32_t>> const data = {{"hotel", 1}, {"homel", 2}, {"hotel", 3}};

  for (auto const & kv : data)
    trie.Add(strings::MakeUniString(kv.first), kv.second);

  trie::MemTrieIterator<Key, ValueList> const rootIterator(trie.GetRootIterator());
  map<uint32_t, bool> vals;
  auto saveResult = [&vals](uint32_t v, bool exactMatch) { vals[v] = exactMatch; };

  auto const hotelDFA = DFA("hotel", 1 /* maxErrors */);
  search::impl::MatchInTrie(rootIterator, nullptr, 0 /* prefixSize */, hotelDFA, saveResult);
  TEST(vals.at(1), (vals));
  TEST(vals.at(3), (vals));
  TEST(!vals.at(2), (vals));

  vals.clear();
  auto const homelDFA = DFA("homel", 1 /* maxErrors */);
  search::impl::MatchInTrie(rootIterator, nullptr, 0 /* prefixSize */, homelDFA, saveResult);
  TEST(vals.at(2), (vals));
  TEST(!vals.at(1), (vals));
  TEST(!vals.at(3), (vals));
}

UNIT_TEST(MatchPrefixInTrieTest)
{
  Trie trie;

  vector<pair<string, uint32_t>> const data = {{"лермонтовъ", 1}, {"лермонтово", 2}};

  for (auto const & kv : data)
    trie.Add(strings::MakeUniString(kv.first), kv.second);

  trie::MemTrieIterator<Key, ValueList> const rootIterator(trie.GetRootIterator());
  map<uint32_t, bool> vals;
  auto saveResult = [&vals](uint32_t v, bool exactMatch) { vals[v] = exactMatch; };

  {
    vals.clear();
    auto const lermontov = PrefixDFA(DFA("лермонтовъ", 2 /* maxErrors */));
    search::impl::MatchInTrie(rootIterator, nullptr, 0 /* prefixSize */, lermontov, saveResult);
    TEST(vals.at(1), (vals));
    TEST(!vals.at(2), (vals));
  }
  {
    vals.clear();
    auto const lermontovo = PrefixDFA(DFA("лермонтово", 2 /* maxErrors */));
    search::impl::MatchInTrie(rootIterator, nullptr, 0 /* prefixSize */, lermontovo, saveResult);
    TEST(vals.at(2), (vals));
    TEST(!vals.at(1), (vals));
  }
  {
    vals.clear();
    auto const commonPrexif = PrefixDFA(DFA("лермонтов", 2 /* maxErrors */));
    search::impl::MatchInTrie(rootIterator, nullptr, 0 /* prefixSize */, commonPrexif, saveResult);
    TEST(vals.at(2), (vals));
    TEST(vals.at(1), (vals));
  }
  {
    vals.clear();
    auto const commonPrexif = PrefixDFA(DFA("лер", 2 /* maxErrors */));
    search::impl::MatchInTrie(rootIterator, nullptr, 0 /* prefixSize */, commonPrexif, saveResult);
    TEST(vals.at(2), (vals));
    TEST(vals.at(1), (vals));
  }
}
} // namespace feature_offset_match_tests
