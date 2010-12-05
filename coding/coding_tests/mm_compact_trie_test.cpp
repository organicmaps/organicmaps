#include "../../testing/testing.hpp"

#include "../byte_stream.hpp"
#include "../compact_trie_builder.hpp"
#include "../dd_compact_trie.hpp"
#include "../reader.hpp"
#include "../../base/base.hpp"
#include "../../base/macros.hpp"
#include "../../std/string.hpp"
#include "../../std/vector.hpp"

//namespace
//{
class DDCompactTrieTester
{
public:
  template <class TChar, class TBitRankDirectory>
      static typename TBitRankDirectory::BitVectorType const &
      IsParentVector(DDCompactTrie<TChar, TBitRankDirectory> const & trie)
  {
    return trie.m_IsParent.BitVector();
  }

  template <class TChar, class TBitRankDirectory>
      static typename TBitRankDirectory::BitVectorType const &
      IsFirstChildVector(DDCompactTrie<TChar, TBitRankDirectory> const & trie)
  {
    return trie.m_IsFirstChild.BitVector();
  }
  template <class TChar, class TBitRankDirectory>
      static int ParentsWithDataCount(
          DDCompactTrie<TChar, TBitRankDirectory> const & trie)
  {
    return trie.m_ParentsWithDataCount;
  }
  };
//}

UNIT_TEST(CompactTrieSimple)
{
  vector<string> words;
  words.push_back("hello");
  words.push_back("help");
  words.push_back("sim");
  words.push_back("simple");
  words.push_back("world");
  words.push_back("z");

  typedef PushBackByteSink<vector<char> > SinkType;
  vector<char> data;
  SinkType sink(data);
  BuildMMCompactTrie<char>(sink, words.begin(), words.end());
  typedef DDBitVector<DDVector<uint32_t, MemReader> > BitVectorDD;
  typedef DDBitRankDirectory<BitVectorDD> RankDirDD;
  typedef DDCompactTrie<char, RankDirDD> TrieDD;
  TrieDD trie;
  MemReader reader(&data[0], data.size());
  {
    DDParseInfo<MemReader> info(reader, true);
    trie.Parse(info);
  }

  // 0    (0)
  // 1   h(1)          s(2)    w(3) z(4)
  // 2   e(5)          i(6)    o(7)
  // 3   l(8)          m(9)    r(10)
  // 4   l(11) p(12)   p(13)   l(14)
  // 5   o(15)         l(16)   d(17)
  // 6                 e(18)

  TEST_EQUAL(trie.NodesWithData(), words.size(), ());
  TEST_EQUAL(trie.Root(), 0U, ());
  TEST_EQUAL(DDCompactTrieTester::ParentsWithDataCount(trie), 1, ());

  //                       0123456789012345678
  char const * chars    = "$hswzeiolmrlpplolde";
  char const * isParent = "1111011111110110100";
  char const * isFChild = "0100011111110111111";
  char const * nexts    = "0111000000010000000";
  //                       0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18
  TrieDD::Id fchild[] = {  1, 5, 6, 7,-1, 8, 9,10,11,13,14,15,-1,16,17,-1,18,-1,-1 };
  TrieDD::Id parent[] = { -1, 0, 0, 0, 0, 1, 2, 3, 5, 6, 7, 8, 8, 9,10,11,13,14,16 };
  TrieDD::Id dataI[]  = { -1,-1,-1,-1, 1,-1,-1,-1,-1, 0,-1,-1, 2,-1,-1, 3,-1, 4, 5 };
  for (size_t i = 0; i <= 17; ++i) {
    // wcout << "!! " << i << endl;
    if (i != 0) {
      TEST_EQUAL(trie.Char(i), chars[i], (i, string(1, trie.Char(i)), string(1, chars[i])));
    }
    TEST_EQUAL(DDCompactTrieTester::IsParentVector(trie)[i],     isParent[i] == '1', (i));
    TEST_EQUAL(DDCompactTrieTester::IsFirstChildVector(trie)[i], isFChild[i] == '1', (i));
    TEST_EQUAL(trie.NextSibling(i), nexts[i] == '1' ? i+1 : trie.INVALID_ID, (i));
    TEST_EQUAL(trie.FirstChild(i), fchild[i], (i));
    TEST_EQUAL(trie.Parent(i), parent[i], (i));
    TEST_EQUAL(trie.Data(i), dataI[i], (i));
  }

}
