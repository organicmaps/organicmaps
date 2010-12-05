#pragma once
#include "dd_bit_vector.hpp"
#include "dd_bit_rank_directory.hpp"
#include "../base/base.hpp"

#include "../base/start_mem_debug.hpp"

template <class BitRankDirT>
class DDCompactTree
{
public:
  typedef BitRankDirT BitRankDirType;

  // Node id.
  typedef typename BitRankDirT::size_type Id;
  static Id const INVALID_ID = -1;

  DDCompactTree()
  {
  }

  // Id of the root.
  Id Root() const
  {
    return 0;
  }

  // Parent id and INVALID_ID for root.
  Id Parent(Id id) const
  {
    ASSERT(id != INVALID_ID, ());
    return id ? m_IsParent.Select1(m_IsFirstChild.Rank1(id)) : INVALID_ID;
  }

  // First child id and INVALID_ID for leaf.
  Id FirstChild(Id id) const
  {
    ASSERT(id != INVALID_ID, ());
    return m_IsParent[id] ? m_IsFirstChild.Select1(m_IsParent.Rank1(id)) : INVALID_ID;
  }

  // Next sibling id and INVALID_ID if there is no next sibling.
  Id NextSibling(Id id) const
  {
    ASSERT(id != INVALID_ID, ());
    return (id + 1 == m_IsFirstChild.size() || m_IsFirstChild[id + 1]) ? INVALID_ID : id + 1;
  }

  template <class ReaderT>
  void Parse(DDParseInfo<ReaderT> & info)
  {
    typedef typename BitRankDirT::BitVectorType BitVectorType;
    {
      BitVectorType isParent;
      isParent.Parse(info);
      if (isParent.size() == 0)
        MYTHROW(DDParseException, ());
      m_IsParent.Parse(info, isParent);
    }
    {
      // TODO: Don't write logRankChunkSize twice.
      // TODO: Allow logRankChunkSize be explicitly specified.
      BitVectorType isFirstChild;
      isFirstChild.Parse(info, m_IsParent.size());
      if (isFirstChild.size() == 0)
        MYTHROW(DDParseException, ());
      m_IsFirstChild.Parse(info, isFirstChild);
    }
  }

protected:
  friend class MMCompactTreeTester;
  BitRankDirT m_IsParent;
  BitRankDirT m_IsFirstChild;
};

template <class BitRankDirT>
class DDCompactTreeWithData : public DDCompactTree<BitRankDirT>
{
public:
  typedef DDCompactTree<BitRankDirT> BaseType;
  typedef BitRankDirT BitRankDirType;
  typedef typename BaseType::Id Id;
  static Id const INVALID_ID = BaseType::INVALID_ID;

  DDCompactTreeWithData() : m_NodesWithData(0)
  {
  }

  // Number of nodes with data.
  Id NodesWithData() const
  {
    return m_NodesWithData;
  }

  // Id of the data for a given node id and INVALID_ID if node doesn't have any data.
  Id Data(Id id) const
  {
    ASSERT(id != INVALID_ID, ());
    if (BaseType::m_IsParent[id])
    {
      Id const parentIndex = BaseType::m_IsParent.Rank1(id) - 1;
      return m_ParentHasData[parentIndex] ? m_ParentHasData.Rank1(parentIndex) - 1 : INVALID_ID;
    }
    else
    {
      return m_ParentsWithDataCount + BaseType::m_IsParent.Rank0(id) - 1;
    }
  }

  template <class ReaderT>
  void Parse(DDParseInfo<ReaderT> & info)
  {
    BaseType::Parse(info);
    // TODO: Pass the vector size here.
    m_ParentHasData.Parse(info);
    m_ParentsWithDataCount =
        m_ParentHasData.empty() ? 0 : m_ParentHasData.Rank1(m_ParentHasData.size() - 1);
    m_NodesWithData = m_ParentsWithDataCount +
                      BaseType::m_IsParent.Rank0(BaseType::m_IsParent.size() - 1);
  }
protected:
  BitRankDirType m_ParentHasData;
  uint32_t m_ParentsWithDataCount;
  size_t m_NodesWithData;
};

#include "../base/stop_mem_debug.hpp"
