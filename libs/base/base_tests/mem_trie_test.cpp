#include "testing/testing.hpp"

#include "base/mem_trie.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace base;
using namespace std;

namespace
{
using Key = string;
using Value = int;
using Trie = MemTrie<Key, VectorValues<Value>>;
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
  Data GetActualContents() const
  {
    Data data;
    m_trie.ForEachInTrie([&data](Key const & k, Value const & v) { data.emplace_back(k, v); });
    sort(data.begin(), data.end());
    return data;
  }

  Data GetExpectedContents() const { return {m_data.cbegin(), m_data.cend()}; }

  vector<Value> GetValuesByKey(Key const & key) const
  {
    vector<Value> values;
    m_trie.ForEachInNode(key, [&](Value const & value) { values.push_back(value); });
    sort(values.begin(), values.end());
    return values;
  }

  Data GetContentsByPrefix(Key const & prefix) const
  {
    Data data;
    m_trie.ForEachInSubtree(prefix, [&data](Key const & k, Value const & v) { data.emplace_back(k, v); });
    sort(data.begin(), data.end());
    return data;
  }

  bool HasKey(Key const & key) const { return m_trie.HasKey(key); }
  bool HasPrefix(Key const & prefix) const { return m_trie.HasPrefix(prefix); }

  size_t GetNumNodes() const { return m_trie.GetNumNodes(); }

  void Add(Key const & key, Value const & value)
  {
    m_trie.Add(key, value);
    m_data.insert(make_pair(key, value));
  }

  void Erase(Key const & key, Value const & value)
  {
    m_trie.Erase(key, value);
    m_data.erase(make_pair(key, value));
  }

protected:
  Trie m_trie;
  multiset<pair<Key, Value>> m_data;
};

UNIT_CLASS_TEST(MemTrieTest, Basic)
{
  TEST_EQUAL(GetNumNodes(), 1, ());

  TEST(!HasKey(""), ());
  TEST(!HasKey("a"), ());
  TEST(!HasPrefix(""), ());
  TEST(!HasPrefix("a"), ());

  Data const data = {{"roger", 3}, {"amy", 1},   {"emma", 1}, {"ann", 1},
                     {"rob", 1},   {"roger", 2}, {"", 0},     {"roger", 1}};
  for (auto const & kv : data)
    Add(kv.first, kv.second);

  TEST_EQUAL(GetNumNodes(), 8, ());
  TEST(HasKey(""), ());
  TEST(!HasKey("a"), ());
  TEST(HasPrefix(""), ());
  TEST(HasPrefix("a"), ());

  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());
  TEST_EQUAL(GetNumNodes(), 8, ());

  Trie newTrie(std::move(m_trie));

  TEST_EQUAL(m_trie.GetNumNodes(), 1, ());
  TEST(GetTrieContents(m_trie).empty(), ());

  TEST_EQUAL(newTrie.GetNumNodes(), 8, ());
  TEST_EQUAL(GetTrieContents(newTrie), GetExpectedContents(), ());
}

UNIT_CLASS_TEST(MemTrieTest, KeysRemoval)
{
  TEST_EQUAL(GetNumNodes(), 1, ());

  TEST(!HasKey("r"), ());
  TEST(!HasPrefix("r"), ());
  TEST(!HasKey("ro"), ());
  TEST(!HasPrefix("ro"), ());

  Data const data = {{"bobby", 1}, {"robby", 2}, {"rob", 3}, {"r", 4}, {"robert", 5}, {"bob", 6}};

  for (auto const & kv : data)
    Add(kv.first, kv.second);

  TEST(HasKey("r"), ());
  TEST(HasPrefix("r"), ());
  TEST(!HasKey("ro"), ());
  TEST(HasPrefix("ro"), ());

  TEST_EQUAL(GetNumNodes(), 7, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("r", 3);
  TEST_EQUAL(GetNumNodes(), 7, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("r", 4);
  TEST_EQUAL(GetNumNodes(), 6, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("robert", 5);
  TEST_EQUAL(GetNumNodes(), 5, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("rob", 3);
  TEST_EQUAL(GetNumNodes(), 4, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());

  Erase("robby", 2);
  TEST_EQUAL(GetNumNodes(), 3, ());
  TEST_EQUAL(GetExpectedContents(), GetActualContents(), ());
}

UNIT_CLASS_TEST(MemTrieTest, ForEachInNode)
{
  Add("abracadabra", 0);
  Add("abra", 1);
  Add("abra", 2);
  Add("abrau", 3);

  TEST_EQUAL(GetValuesByKey("a"), vector<Value>{}, ());
  TEST_EQUAL(GetValuesByKey("abrac"), vector<Value>{}, ());
  TEST_EQUAL(GetValuesByKey("abracadabr"), vector<Value>{}, ());
  TEST_EQUAL(GetValuesByKey("void"), vector<Value>{}, ());

  TEST_EQUAL(GetValuesByKey("abra"), vector<Value>({1, 2}), ());
  TEST_EQUAL(GetValuesByKey("abracadabra"), vector<Value>({0}), ());
  TEST_EQUAL(GetValuesByKey("abrau"), vector<Value>({3}), ());
}

UNIT_CLASS_TEST(MemTrieTest, ForEachInSubtree)
{
  Add("abracadabra", 0);
  Add("abra", 1);
  Add("abra", 2);
  Add("abrau", 3);

  Data const all = {{"abra", 1}, {"abra", 2}, {"abracadabra", 0}, {"abrau", 3}};

  TEST_EQUAL(GetContentsByPrefix(""), all, ());
  TEST_EQUAL(GetContentsByPrefix("a"), all, ());
  TEST_EQUAL(GetContentsByPrefix("abra"), all, ());
  TEST_EQUAL(GetContentsByPrefix("abracadabr"), Data({{"abracadabra", 0}}), ());
  TEST_EQUAL(GetContentsByPrefix("abracadabra"), Data({{"abracadabra", 0}}), ());
  TEST_EQUAL(GetContentsByPrefix("void"), Data{}, ());
  TEST_EQUAL(GetContentsByPrefix("abra"), all, ());
  TEST_EQUAL(GetContentsByPrefix("abrau"), Data({{"abrau", 3}}), ());
}
}  // namespace
