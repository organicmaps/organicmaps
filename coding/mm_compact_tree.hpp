#pragma once
#include "mm_bit_vector.hpp"
#include "../base/base.hpp"

class MMCompactTree
{
public:
  // Node id.
  typedef uint32_t Id;
  static Id const INVALID_ID = 0xFFFFFFFF;

  MMCompactTree() :
      m_IsParentDir(m_IsParent), m_IsFirstChildDir(m_IsFirstChild)
  {
  }

  MMCompactTree(void const * p, size_t size) :
      m_IsParentDir(m_IsParent), m_IsFirstChildDir(m_IsFirstChild)
  {
    MMParseInfo info(p, size, true);
    this->Parse(info);
  }

  // Id of the root.
  Id Root() const
  {
    return 0;
  }

  // Parent id and INVALID_ID for root.
  Id Parent(Id id) const
  {
    return id ? m_IsParentDir.Select1(m_IsFirstChildDir.Rank1(id)) : INVALID_ID;
  }

  // First child id and INVALID_ID for leaf.
  Id FirstChild(Id id) const
  {
    return m_IsParent[id] ? m_IsFirstChildDir.Select1(m_IsParentDir.Rank1(id)) : INVALID_ID;
  }

  // Next sibling id and INVALID_ID if there is no next sibling.
  Id NextSibling(Id id) const
  {
    return (id + 1 == m_IsFirstChild.size() || m_IsFirstChild[id + 1]) ? INVALID_ID : id + 1;
  }

  void Parse(MMParseInfo & info)
  {
    if (!info.Successful())
      return;
    m_IsParent.Parse(info);
    m_IsParentDir.Parse(info);
    CHECK_OR_CALL(info.FailOnError(), info.Fail, m_IsParent.size() > 0, ());
    m_IsFirstChild.Parse(info, m_IsParent.size());
    m_IsFirstChildDir.Parse(info);
  }

protected:
  friend class MMCompactTreeTester;
  MMBitVector<uint32_t> m_IsParent;
  MMBitVector32RankDirectory m_IsParentDir;
  MMBitVector<uint32_t> m_IsFirstChild;
  MMBitVector32RankDirectory m_IsFirstChildDir;
};

class MMCompactTreeWithData : public MMCompactTree
{
public:
  MMCompactTreeWithData() : m_ParentHasDataDir(m_ParentHasData), m_Size(0)
  {
  }

  // Number of nodes with data.
  size_t NodesWithData()
  {
    return m_Size;
  }

  // Id of the data for a given node id and INVALID_ID if node doesn't have any data.
  uint32_t Data(Id id) const
  {
    if (m_IsParent[id])
    {
      uint32_t const parentIndex = m_IsParentDir.Rank1(id) - 1;
      return m_ParentHasData[parentIndex] ? m_ParentHasDataDir.Rank1(parentIndex) - 1 : INVALID_ID;
    }
    else
    {
      return m_ParentsWithDataCount + m_IsParentDir.Rank0(id) - 1;
    }
  }

  void Parse(MMParseInfo & info)
  {
    MMCompactTree::Parse(info);
    if (!info.Successful())
      return;
    m_ParentHasData.Parse(
        info, m_IsParent.empty() ? 0 : m_IsParentDir.Rank1(m_IsParent.size() - 1));
    m_ParentHasDataDir.Parse(info);
    m_ParentsWithDataCount =
        m_ParentHasData.empty() ? 0 : m_ParentHasDataDir.Rank1(m_ParentHasData.size() - 1);
    m_Size = m_ParentsWithDataCount +
             (m_IsParent.empty() ? 0 : m_IsParentDir.Rank0(m_IsParent.size() - 1));
  }
protected:
  MMBitVector<uint32_t> m_ParentHasData;
  MMBitVector32RankDirectory m_ParentHasDataDir;
  uint32_t m_ParentsWithDataCount;
  size_t m_Size;

};
