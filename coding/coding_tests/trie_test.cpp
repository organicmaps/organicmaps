#include "testing/testing.hpp"
#include "coding/trie.hpp"
#include "coding/trie_builder.hpp"
#include "coding/trie_reader.hpp"
#include "coding/byte_stream.hpp"
#include "coding/write_to_sink.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/cstring.hpp"

#include <boost/utility/binary.hpp>


namespace
{

struct ChildNodeInfo
{
  bool m_isLeaf;
  uint32_t m_size;
  vector<uint32_t> m_edge;
  string m_edgeValue;
  ChildNodeInfo(bool isLeaf, uint32_t size, char const * edge, char const * edgeValue)
    : m_isLeaf(isLeaf), m_size(size), m_edgeValue(edgeValue)
  {
    while (*edge)
      m_edge.push_back(*edge++);
  }

  uint32_t Size() const { return m_size; }
  bool IsLeaf() const { return m_isLeaf; }
  uint32_t const * GetEdge() const { return &m_edge[0]; }
  uint32_t GetEdgeSize() const { return m_edge.size(); }
  void const * GetEdgeValue() const { return m_edgeValue.data(); }
  uint32_t GetEdgeValueSize() const { return m_edgeValue.size(); }
};

struct KeyValuePair
{
  buffer_vector<trie::TrieChar, 8> m_key;
  uint32_t m_value;

  KeyValuePair() {}

  template <class TString>
  KeyValuePair(TString const & key, int value)
    : m_key(key.begin(), key.end()), m_value(value)
  {}

  uint32_t GetKeySize() const { return m_key.size(); }
  trie::TrieChar const * GetKeyData() const { return m_key.data(); }
  uint32_t GetValue() const { return m_value; }

  inline void const * value_data() const { return &m_value; }

  inline size_t value_size() const { return sizeof(m_value); }

  bool operator == (KeyValuePair const & p) const
  {
    return (m_key == p.m_key && m_value == p.m_value);
  }

  bool operator < (KeyValuePair const & p) const
  {
    return ((m_key != p.m_key) ? m_key < p.m_key : m_value < p.m_value);
  }

  void Swap(KeyValuePair & r)
  {
    m_key.swap(r.m_key);
    swap(m_value, r.m_value);
  }
};

string DebugPrint(KeyValuePair const & p)
{
  string keyS = ::DebugPrint(p.m_key);
  ostringstream out;
  out << "KVP(" << keyS << ", " << p.m_value << ")";
  return out.str();
}

struct KeyValuePairBackInserter
{
  vector<KeyValuePair> m_v;
  template <class TString>
  void operator()(TString const & s, trie::FixedSizeValueReader<4>::ValueType const & rawValue)
  {
    uint32_t value;
    memcpy(&value, &rawValue, 4);
    m_v.push_back(KeyValuePair(s, value));
  }
};

struct MaxValueCalc
{
  using ValueType = uint8_t;

  ValueType operator() (void const * p, uint32_t size) const
  {
    ASSERT_EQUAL(size, 4, ());
    uint32_t value;
    memcpy(&value, p, 4);
    ASSERT_LESS(value, 256, ());
    return static_cast<uint8_t>(value);
  }
};

class CharValueList
{
public:
  CharValueList(const string & s) : m_string(s) {}

  size_t size() const { return m_string.size(); }

  bool empty() const { return m_string.empty(); }

  template <typename TSink>
  void Dump(TSink & sink) const
  {
    sink.Write(m_string.data(), m_string.size());
  }

private:
  string m_string;
};

class Uint32ValueList
{
public:
  using TBuffer = vector<uint32_t>;

  void Append(uint32_t value)
  {
    m_values.push_back(value);
  }

  uint32_t size() const { return m_values.size(); }

  bool empty() const { return m_values.empty(); }

  template <typename TSink>
  void Dump(TSink & sink) const
  {
    sink.Write(m_values.data(), m_values.size() * sizeof(TBuffer::value_type));
  }

private:
  TBuffer m_values;
};

}  // unnamed namespace

#define ZENC bits::ZigZagEncode
#define MKSC(x) static_cast<signed char>(x)
#define MKUC(x) static_cast<uint8_t>(x)

UNIT_TEST(TrieBuilder_WriteNode_Smoke)
{
  vector<uint8_t> serial;
  PushBackByteSink<vector<uint8_t> > sink(serial);
  ChildNodeInfo children[] =
  {
    ChildNodeInfo(true, 1, "1A", "i1"),
    ChildNodeInfo(false, 2, "B", "ii2"),
    ChildNodeInfo(false, 3, "zz", ""),
    ChildNodeInfo(true, 4,
                  "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij", "i4"),
    ChildNodeInfo(true, 5, "a", "5z")
  };

  CharValueList valueList("123");
  trie::WriteNode(sink, 0, valueList, &children[0], &children[0] + ARRAY_SIZE(children));
  uint8_t const expected [] =
  {
    BOOST_BINARY(11000101),                                                 // Header: [0b11] [0b000101]
    3,                                                                      // Number of values
    '1', '2', '3',                                                          // Values
    BOOST_BINARY(10000001),                                                 // Child 1: header: [+leaf] [-supershort]  [2 symbols]
    MKUC(ZENC(MKSC('1'))), MKUC(ZENC(MKSC('A') - MKSC('1'))),               // Child 1: edge
    'i', '1',                                                               // Child 1: intermediate data
    1,                                                                      // Child 1: size
    MKUC(64 | ZENC(MKSC('B') - MKSC('1'))),                                 // Child 2: header: [-leaf] [+supershort]
    'i', 'i', '2',                                                          // Child 2: intermediate data
    2,                                                                      // Child 2: size
    BOOST_BINARY(00000001),                                                 // Child 3: header: [-leaf] [-supershort]  [2 symbols]
    MKUC(ZENC(MKSC('z') - MKSC('B'))), 0,                                   // Child 3: edge
    3,                                                                      // Child 3: size
    BOOST_BINARY(10111111),                                                 // Child 4: header: [+leaf] [-supershort]  [>= 63 symbols]
    69,                                                                     // Child 4: edgeSize - 1
    MKUC(ZENC(MKSC('a') - MKSC('z'))), 2,2,2,2,2,2,2,2,2,                   // Child 4: edge
    MKUC(ZENC(MKSC('a') - MKSC('j'))), 2,2,2,2,2,2,2,2,2,                   // Child 4: edge
    MKUC(ZENC(MKSC('a') - MKSC('j'))), 2,2,2,2,2,2,2,2,2,                   // Child 4: edge
    MKUC(ZENC(MKSC('a') - MKSC('j'))), 2,2,2,2,2,2,2,2,2,                   // Child 4: edge
    MKUC(ZENC(MKSC('a') - MKSC('j'))), 2,2,2,2,2,2,2,2,2,                   // Child 4: edge
    MKUC(ZENC(MKSC('a') - MKSC('j'))), 2,2,2,2,2,2,2,2,2,                   // Child 4: edge
    MKUC(ZENC(MKSC('a') - MKSC('j'))), 2,2,2,2,2,2,2,2,2,                   // Child 4: edge
    'i', '4',                                                               // Child 4: intermediate data
    4,                                                                      // Child 4: size
    MKUC(BOOST_BINARY(11000000) | ZENC(0)),                                 // Child 5: header: [+leaf] [+supershort]
    '5', 'z'                                                                // Child 5: intermediate data
  };

  TEST_EQUAL(serial, vector<uint8_t>(&expected[0], &expected[0] + ARRAY_SIZE(expected)), ());
}

UNIT_TEST(TrieBuilder_Build)
{
  int const base = 3;
  int const maxLen = 3;

  vector<string> possibleStrings(1, string());
  for (int len = 1; len <= maxLen; ++len)
  {
    for (int i = 0, p = static_cast<int>(pow((double) base, len)); i < p; ++i)
    {
      string s(len, 'A');
      int t = i;
      for (int l = len - 1; l >= 0; --l, t /= base)
        s[l] += (t % base);
      possibleStrings.push_back(s);
    }
  }
  sort(possibleStrings.begin(), possibleStrings.end());
  // LOG(LINFO, (possibleStrings));

  int const count = static_cast<int>(possibleStrings.size());
  for (int i0 = -1; i0 < count; ++i0)
    for (int i1 = i0; i1 < count; ++i1)
      for (int i2 = i1; i2 < count; ++i2)
  {
    vector<KeyValuePair> v;
    if (i0 >= 0) v.push_back(KeyValuePair(possibleStrings[i0], i0));
    if (i1 >= 0) v.push_back(KeyValuePair(possibleStrings[i1], i1 + 10));
    if (i2 >= 0) v.push_back(KeyValuePair(possibleStrings[i2], i2 + 100));
    vector<string> vs;
    for (size_t i = 0; i < v.size(); ++i)
      vs.push_back(string(v[i].m_key.begin(), v[i].m_key.end()));

    vector<uint8_t> serial;
    PushBackByteSink<vector<uint8_t> > sink(serial);
    trie::Build<PushBackByteSink<vector<uint8_t>>, typename vector<KeyValuePair>::iterator,
                trie::MaxValueEdgeBuilder<MaxValueCalc>, Uint32ValueList>(
        sink, v.begin(), v.end(), trie::MaxValueEdgeBuilder<MaxValueCalc>());
    reverse(serial.begin(), serial.end());
    // LOG(LINFO, (serial.size(), vs));

    MemReader memReader = MemReader(&serial[0], serial.size());
    using IteratorType = trie::Iterator<trie::FixedSizeValueReader<4>::ValueType,
                                        trie::FixedSizeValueReader<1>::ValueType>;
    unique_ptr<IteratorType> const root(trie::ReadTrie(memReader, trie::FixedSizeValueReader<4>(),
                                                       trie::FixedSizeValueReader<1>()));
    vector<KeyValuePair> res;
    KeyValuePairBackInserter f;
    trie::ForEachRef(*root, f, vector<trie::TrieChar>());
    sort(f.m_v.begin(), f.m_v.end());
    TEST_EQUAL(v, f.m_v, ());

    uint32_t expectedMaxEdgeValue = 0;
    for (size_t i = 0; i < v.size(); ++i)
      if (!v[i].m_key.empty())
        expectedMaxEdgeValue = max(expectedMaxEdgeValue, v[i].m_value);
    uint32_t maxEdgeValue = 0;
    for (uint32_t i = 0; i < root->m_edge.size(); ++i)
      maxEdgeValue = max(maxEdgeValue, static_cast<uint32_t>(root->m_edge[i].m_value.m_data[0]));
    TEST_EQUAL(maxEdgeValue, expectedMaxEdgeValue, (v, f.m_v));
  }
}
