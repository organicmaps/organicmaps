#pragma once
#include "mm_base.hpp"
#include "mm_compact_tree.hpp"
#include "mm_vector.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../std/string.hpp"
#include "../base/start_mem_debug.hpp"

class MMCompactTrieTester;

template <class TChar> class MMCompactTrie : public MMCompactTreeWithData
{
public:
  MMCompactTrie()
  {
  }

  MMCompactTrie(void const * p, size_t size) : MMCompactTreeWithData()
  {
    MMParseInfo info(p, size, true);
    Parse(info);
  }

  TChar Char(Id id) const
  {
    ASSERT(id != 0, ());
    ASSERT(id != INVALID_ID, ());
    return m_Chars[id - 1];  // There is no char for the root.
  }

  void Parse(MMParseInfo & info)
  {
    MMCompactTreeWithData::Parse(info);
    if (!info.Successful())
      return;
    m_Chars.Parse(info, m_IsParent.size() - 1);
  }

protected:
  friend class MMCompactTrieTester;
  MMVector<TChar> m_Chars;
};

#if 0
template <class TrieT, typename ItT>
MMCompactTree::Id FindNodeByPath(TrieT const & trie, ItT pathBegin, ItT pathEnd)
{
  MMCompactTree::Id nodeId = 0;
  for (ItT edge = pathBegin; edge != pathEnd; ++edge)
  {
    bool found = false;
    for (MMCompactTree::Id child = trie.FirstChild(nodeId);
         child != MMCompactTree::INVALID_ID;
         child = trie.NextSibling(child))
    {
      if (trie.Char(child) == *edge)
      {
        nodeId = child;
        found = true;
        break;
      }
    }
    if (!found) return MMCompactTree::INVALID_ID;
  }
  return nodeId;
}
#endif

#include "../base/stop_mem_debug.hpp"
