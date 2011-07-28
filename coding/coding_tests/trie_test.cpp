#include "../../testing/testing.hpp"
#include "../trie.hpp"
#include "../trie_builder.hpp"
#include "../byte_stream.hpp"
#include "../write_to_sink.hpp"
#include "../../std/vector.hpp"
#include <boost/utility/binary.hpp>

namespace
{

struct ChildNodeInfo
{
  bool m_isLeaf;
  uint32_t m_size;
  char const * m_edge;
  uint32_t Size() const { return m_size; }
  bool IsLeaf() const { return m_isLeaf; }
  strings::UniString GetEdge() const { return strings::MakeUniString(m_edge); }
};

}  // unnamed namespace

#define ZENC bits::ZigZagEncode

UNIT_TEST(TrieBuilder_WriteNode_Smoke)
{
  vector<uint8_t> serial;
  PushBackByteSink<vector<uint8_t> > sink(serial);
  ChildNodeInfo children[] =
  {
    {true, 1, "1A"},
    {false, 2, "B"},
    {false, 3, "zz"},
    {true, 4, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij"}
  };
  trie::builder::WriteNode(sink, 0, 3, "123", 3,
                           &children[0], &children[0] + ARRAY_SIZE(children));
  unsigned char const expected [] =
  {
    BOOST_BINARY(11000100),             // Header: [0b11] [0b000100]
    3,                                  // Number of values
    '1', '2', '3',                      // Values
    1,                                  // Child 1: size
    BOOST_BINARY(10000010),             // Child 1: header: [+leaf] [-supershort]  [2 symbols]
    ZENC('1'), ZENC('A' - '1'),         // Child 1: edge
    2,                                  // Child 2: size
    64 | ZENC('B' - '1'),               // Child 2: header: [-leaf] [+supershort]
    3,                                  // Child 3: size
    BOOST_BINARY(00000010),             // Child 3: header: [-leaf] [-supershort]  [2 symbols]
    ZENC('z' - 'B'), 0,                 // Child 3: edge
    4,                                  // Child 4: size
    BOOST_BINARY(10111111),             // Child 4: header: [+leaf] [-supershort]  [>= 63 symbols]
    70,                                 // Child 4: edge size
    ZENC('a' - 'z'), 2,2,2,2,2,2,2,2,2, // Child 4: edge
    ZENC('a' - 'j'), 2,2,2,2,2,2,2,2,2, // Child 4: edge
    ZENC('a' - 'j'), 2,2,2,2,2,2,2,2,2, // Child 4: edge
    ZENC('a' - 'j'), 2,2,2,2,2,2,2,2,2, // Child 4: edge
    ZENC('a' - 'j'), 2,2,2,2,2,2,2,2,2, // Child 4: edge
    ZENC('a' - 'j'), 2,2,2,2,2,2,2,2,2, // Child 4: edge
    ZENC('a' - 'j'), 2,2,2,2,2,2,2,2,2  // Child 4: edge
  };

  TEST_EQUAL(serial, vector<uint8_t>(&expected[0], &expected[0] + ARRAY_SIZE(expected)), ());
}
