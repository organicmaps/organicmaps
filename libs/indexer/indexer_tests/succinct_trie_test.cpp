#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "indexer/succinct_trie_builder.hpp"
#include "indexer/succinct_trie_reader.hpp"
#include "indexer/trie.hpp"
#include "indexer/trie_builder.hpp"
#include "indexer/trie_reader.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace
{
struct StringsFileEntryMock
{
  StringsFileEntryMock() = default;
  StringsFileEntryMock(string const & key, uint8_t value) : m_key(key.begin(), key.end()), m_value(value) {}

  trie::TrieChar const * GetKeyData() { return m_key.data(); }

  size_t GetKeySize() const { return m_key.size(); }

  uint8_t GetValue() const { return m_value; }
  void * value_data() const { return nullptr; }
  size_t value_size() const { return 0; }

  void Swap(StringsFileEntryMock & o)
  {
    swap(m_key, o.m_key);
    swap(m_value, o.m_value);
  }

  bool operator==(StringsFileEntryMock const & o) const { return m_key == o.m_key && m_value == o.m_value; }

  bool operator<(StringsFileEntryMock const & o) const
  {
    if (m_key != o.m_key)
      return m_key < o.m_key;
    return m_value < o.m_value;
  }

  vector<trie::TrieChar> m_key;
  uint8_t m_value;
};

template <typename TWriter>
struct EmptyValueList
{
  size_t size() const { return 0; }
  void Dump(TWriter &) const {}
  void Append(int) {}
};

struct EmptyValueReader
{
  using ValueType = unsigned char;

  EmptyValueReader() = default;

  template <typename SourceT>
  void operator()(SourceT &, ValueType & value) const
  {
    value = 0;
  }
};

struct SimpleValueReader
{
public:
  SimpleValueReader() = default;

  using ValueType = uint8_t;

  template <typename TReader>
  void operator()(TReader & reader, ValueType & v) const
  {
    v = ReadPrimitiveFromSource<uint8_t>(reader);
  }

  template <class TWriter>
  void Save(TWriter & writer, ValueType const & v) const
  {
    WriteToSink(writer, v);
  }
};

template <typename TWriter>
struct SimpleValueList
{
  size_t size() const { return m_valueList.size(); }

  void Dump(TWriter & writer) const
  {
    for (uint8_t x : m_valueList)
      WriteToSink(writer, x);
  }

  void Append(uint8_t x) { m_valueList.push_back(x); }

  vector<uint8_t> m_valueList;
};

using TSimpleIterator = trie::SuccinctTrieIterator<MemReader, SimpleValueReader>;

void ReadAllValues(TSimpleIterator & root, vector<uint8_t> & values)
{
  for (size_t i = 0; i < root.NumValues(); ++i)
    values.push_back(root.GetValue(i));
}

void CollectInSubtree(TSimpleIterator & root, vector<uint8_t> & collectedValues)
{
  ReadAllValues(root, collectedValues);

  if (auto l = root.GoToEdge(0))
    CollectInSubtree(*l, collectedValues);
  if (auto r = root.GoToEdge(1))
    CollectInSubtree(*r, collectedValues);
}

template <typename TWriter>
void BuildFromSimpleValueList(TWriter & writer, vector<StringsFileEntryMock> & data)
{
  trie::BuildSuccinctTrie<TWriter, vector<StringsFileEntryMock>::iterator, SimpleValueList<TWriter>>(
      writer, data.begin(), data.end());
}
}  // namespace

namespace trie
{
// todo(@pimenov): It may be worth it to write a test
// for the trie's topology but the test may be flaky because
// it has to depend on the particular Huffman encoding of the key strings.
// This is remedied by separation of the string-encoding and trie-building
// parts, but they are not separated now, hence there is no such test.

UNIT_TEST(SuccinctTrie_Serialization_Smoke1)
{
  vector<uint8_t> buf;

  using TWriter = MemWriter<vector<uint8_t>>;
  TWriter memWriter(buf);

  vector<StringsFileEntryMock> data = {StringsFileEntryMock("abacaba", 1)};

  trie::BuildSuccinctTrie<TWriter, vector<StringsFileEntryMock>::iterator, EmptyValueList<TWriter>>(
      memWriter, data.begin(), data.end());

  MemReader memReader(buf.data(), buf.size());

  auto trieRoot = trie::ReadSuccinctTrie(memReader, EmptyValueReader());
  TEST(trieRoot, ());
}

UNIT_TEST(SuccinctTrie_Serialization_Smoke2)
{
  vector<uint8_t> buf;

  using TWriter = MemWriter<vector<uint8_t>>;
  TWriter memWriter(buf);

  vector<StringsFileEntryMock> data = {StringsFileEntryMock("abacaba", 1)};

  BuildFromSimpleValueList(memWriter, data);

  MemReader memReader(buf.data(), buf.size());

  auto trieRoot = trie::ReadSuccinctTrie(memReader, SimpleValueReader());
  TEST(trieRoot, ());
}

UNIT_TEST(SuccinctTrie_Iterator)
{
  vector<uint8_t> buf;

  using TWriter = MemWriter<vector<uint8_t>>;
  TWriter memWriter(buf);

  vector<StringsFileEntryMock> data = {StringsFileEntryMock("a", 1), StringsFileEntryMock("b", 2),
                                       StringsFileEntryMock("ab", 3), StringsFileEntryMock("ba", 4),
                                       StringsFileEntryMock("abc", 5)};
  sort(data.begin(), data.end());

  BuildFromSimpleValueList(memWriter, data);

  MemReader memReader(buf.data(), buf.size());

  auto trieRoot = trie::ReadSuccinctTrie(memReader, SimpleValueReader());
  TEST(trieRoot, ());

  vector<uint8_t> collectedValues;
  CollectInSubtree(*trieRoot, collectedValues);
  sort(collectedValues.begin(), collectedValues.end());
  TEST_EQUAL(collectedValues.size(), 5, ());
  for (size_t i = 0; i < collectedValues.size(); ++i)
    TEST_EQUAL(collectedValues[i], i + 1, ());
}

UNIT_TEST(SuccinctTrie_MoveToString)
{
  vector<uint8_t> buf;

  using TWriter = MemWriter<vector<uint8_t>>;
  TWriter memWriter(buf);

  vector<StringsFileEntryMock> data = {StringsFileEntryMock("abcde", 1), StringsFileEntryMock("aaaaa", 2),
                                       StringsFileEntryMock("aaa", 3), StringsFileEntryMock("aaa", 4)};
  sort(data.begin(), data.end());

  BuildFromSimpleValueList(memWriter, data);
  MemReader memReader(buf.data(), buf.size());

  auto trieRoot = trie::ReadSuccinctTrie(memReader, SimpleValueReader());

  {
    auto it = trieRoot->GoToString(strings::MakeUniString("a"));
    TEST(it != nullptr, ());
    vector<uint8_t> expectedValues;
    vector<uint8_t> receivedValues;
    ReadAllValues(*it.get(), receivedValues);
    TEST_EQUAL(expectedValues, receivedValues, ());
  }

  {
    auto it = trieRoot->GoToString(strings::MakeUniString("abcde"));
    TEST(it != nullptr, ());
    vector<uint8_t> expectedValues{1};
    vector<uint8_t> receivedValues;
    ReadAllValues(*it.get(), receivedValues);
    TEST_EQUAL(expectedValues, receivedValues, ());
  }

  {
    auto it = trieRoot->GoToString(strings::MakeUniString("aaaaa"));
    TEST(it != nullptr, ());
    vector<uint8_t> expectedValues{2};
    vector<uint8_t> receivedValues;
    ReadAllValues(*it.get(), receivedValues);
    TEST_EQUAL(expectedValues, receivedValues, ());
  }

  {
    auto it = trieRoot->GoToString(strings::MakeUniString("aaa"));
    TEST(it != nullptr, ());
    vector<uint8_t> expectedValues{3, 4};
    vector<uint8_t> receivedValues;
    ReadAllValues(*it.get(), receivedValues);
    TEST_EQUAL(expectedValues, receivedValues, ());
  }

  {
    auto it = trieRoot->GoToString(strings::MakeUniString("b"));
    TEST(it == nullptr, ());
  }

  {
    auto it = trieRoot->GoToString(strings::MakeUniString("bbbbb"));
    TEST(it == nullptr, ());
  }
}

}  // namespace trie
