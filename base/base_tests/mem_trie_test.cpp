#include "testing/testing.hpp"

#include "base/mem_trie.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace
{
using Key = string;
using Value = int;
using Trie = my::MemTrie<Key, my::VectorValues<Value>>;
using Data = vector<pair<Key, Value>>;

Data GetTrieContents(Trie const & trie)
{
  Data data;
  trie.ForEachInTrie([&data](string const & k, int v) { data.emplace_back(k, v); });
  sort(data.begin(), data.end());
  return data;
}

class MemTrieTest
{
public:
  Data GetActualContents() const { return ::GetTrieContents(m_trie); }

  Data GetExpectedContents() const { return m_data; }

  size_t GetNumNodes() const { return m_trie.GetNumNodes(); }

  void Add(Key const & key, Value const & value)
  {
    m_trie.Add(key, value);

    auto const kv = make_pair(key, value);
    auto it = lower_bound(m_data.begin(), m_data.end(), kv);
    m_data.insert(it, kv);
  }

  void Erase(Key const & key, Value const & value)
  {
    m_trie.Erase(key, value);

    auto const kv = make_pair(key, value);
    auto it = lower_bound(m_data.begin(), m_data.end(), kv);
    if (it != m_data.end() && *it == kv)
      m_data.erase(it);
  }

protected:
  Trie m_trie;
  Data m_data;
};

UNIT_CLASS_TEST(MemTrieTest, Basic)
{
  TEST_EQUAL(GetNumNodes(), 1, ());

  Data const data = {{"roger", 3}, {"amy", 1},   {"emma", 1}, {"ann", 1},
                     {"rob", 1},   {"roger", 2}, {"", 0},     {"roger", 1}};
  for (auto const & kv : data)
    Add(kv.first, kv.second);
  TEST_EQUAL(GetNumNodes(), 16, ());

  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());
  TEST_EQUAL(GetNumNodes(), 16, ());

  Trie newTrie(move(m_trie));

  TEST_EQUAL(m_trie.GetNumNodes(), 1, ());
  TEST(GetTrieContents(m_trie).empty(), ());

  TEST_EQUAL(newTrie.GetNumNodes(), 16, ());
  TEST_EQUAL(GetTrieContents(newTrie), GetExpectedContents(), ());
}

UNIT_CLASS_TEST(MemTrieTest, KeysRemoval)
{
  TEST_EQUAL(GetNumNodes(), 1, ());

  Data const data = {{"bobby", 1}, {"robby", 2}, {"rob", 3}, {"r", 4}, {"robert", 5}, {"bob", 6}};

  for (auto const & kv : data)
    Add(kv.first, kv.second);

  TEST_EQUAL(GetNumNodes(), 14, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("r", 3);
  TEST_EQUAL(GetNumNodes(), 14, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("r", 4);
  TEST_EQUAL(GetNumNodes(), 14, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("robert", 5);
  TEST_EQUAL(GetNumNodes(), 11, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("rob", 3);
  TEST_EQUAL(GetNumNodes(), 11, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("robby", 2);
  TEST_EQUAL(GetNumNodes(), 6, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());
}
}  // namespace
