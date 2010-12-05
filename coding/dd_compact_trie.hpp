#pragma once
#include "dd_compact_tree.hpp"
#include "dd_vector.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../std/string.hpp"
#include "../base/start_mem_debug.hpp"

class MMCompactTTrieester;

template <class TChar, class BitRankDirT>
class DDCompactTrie : public DDCompactTreeWithData<BitRankDirT>
{
public:
  typedef DDCompactTreeWithData<BitRankDirT> BaseType;
  typedef typename BaseType::Id Id;
  static Id const INVALID_ID = BaseType::INVALID_ID;

  DDCompactTrie()
  {
  }

  TChar Char(Id id) const
  {
    ASSERT(id != 0, ());
    ASSERT(id != INVALID_ID, ());
    return m_Chars[id - 1];  // There is no char for the root.
  }

  template <class ReaderT>
  void Parse(DDParseInfo<ReaderT> & info)
  {
    BaseType::Parse(info);
    m_Chars =
        CharVectorType(info.Source().SubReader((BaseType::m_IsParent.size() - 1) * sizeof(TChar)),
                       BaseType::m_IsParent.size() - 1);
  }

protected:
  friend class DDCompactTrieTester;
  typedef DDVector<TChar, typename BaseType::BitRankDirType::BitVectorType::VectorType::ReaderType>
      CharVectorType;
  CharVectorType m_Chars;
};

template <class TTrie, typename TIter>
typename TTrie::Id FindNodeByPath(TTrie const & trie, TIter pathBegin, TIter pathEnd)
{
  typename TTrie::Id nodeId = trie.Root();
  for (TIter edge = pathBegin; edge != pathEnd; ++edge)
  {
    bool found = false;
    for (typename TTrie::Id child = trie.FirstChild(nodeId);
         child != TTrie::INVALID_ID;
         child = trie.NextSibling(child))
    {
      if (trie.Char(child) == *edge)
      {
        nodeId = child;
        found = true;
        break;
      }
    }
    if (!found)
      return TTrie::INVALID_ID;
  }
  return nodeId;
}

#include "../base/stop_mem_debug.hpp"
