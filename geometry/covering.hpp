#pragma once

#include "geometry/covering_utils.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/logging.hpp"
#include "base/set_operations.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/array.hpp"
#include "std/functional.hpp"
#include "std/iterator.hpp"
#include "std/map.hpp"
#include "std/queue.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace covering
{
template <class CellIdT>
class Covering
{
public:
  typedef CellIdT CellId;
  typedef typename CellId::LessLevelOrder LessLevelOrder;

  Covering() : m_Size(0) {}

  explicit Covering(CellId cell) : m_Size(1) { m_Covering[cell.Level()].push_back(cell); }

  explicit Covering(vector<CellId> const & v)
  {
    for (size_t i = 0; i < v.size(); ++i)
      m_Covering[v[i].Level()].push_back(v[i]);
    Sort();
    Unique();
    RemoveDuplicateChildren();
    RemoveFullSquares();
    m_Size = CalculateSize();
  }

  // Cover triangle.
  Covering(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c,
           int level = CellId::DEPTH_LEVELS - 1)
  {
    // TODO: ASSERT(a != b), ASSERT(b != c), ASSERT(a != c) ?
    ASSERT(0 <= level && level <= CellId::DEPTH_LEVELS, (level, CellId::Root()));
    CoverTriangleInfo info;
    info.m_pCovering = this;
    info.m_A = a;
    info.m_B = b;
    info.m_C = c;
    info.m_Level = level;
    CoverTriangleImpl(info, CellId::Root());
    Sort();
    RemoveFullSquares();
    m_Size = CalculateSize();
  }

  size_t Size() const
  {
    ASSERT_EQUAL(m_Size, CalculateSize(), ());
    return m_Size;
  }

  void Append(Covering<CellId> const & c)
  {
    AppendWithoutNormalize(c);
    RemoveDuplicateChildren();
    RemoveFullSquares();
    m_Size = CalculateSize();
  }

  void OutputToVector(vector<CellId> & result) const
  {
    for (int level = 0; level < CellId::DEPTH_LEVELS; ++level)
      result.insert(result.end(), m_Covering[level].begin(), m_Covering[level].end());
  }

  void OutputToVector(vector<int64_t> & result, int cellDepth) const
  {
    for (int level = 0; level < CellId::DEPTH_LEVELS; ++level)
      for (size_t i = 0; i < m_Covering[level].size(); ++i)
        result.push_back(m_Covering[level][i].ToInt64(cellDepth));
  }

  void Simplify()
  {
    size_t cellsSimplified = 0;
    auto const initialSize = m_Size;
    for (int level = CellId::DEPTH_LEVELS - 1; level > 1; --level)
    {
      if (m_Covering[level].size() >= 2)
      {
        auto const initialLevelSize = m_Covering[level].size();
        SimplifyLevel(level);
        ASSERT_GREATER_OR_EQUAL(initialLevelSize, m_Covering[level].size(), ());
        cellsSimplified += initialLevelSize - m_Covering[level].size();
        if (cellsSimplified > initialSize / 2)
          break;
      }
    }
    RemoveDuplicateChildren();
    RemoveFullSquares();
    m_Size = CalculateSize();
  }

private:
  void SimplifyLevel(int level)
  {
    map<CellId, uint32_t, LessLevelOrder> parentCellCounts;
    typedef typename vector<CellId>::const_iterator ConstIteartor;
    for (ConstIteartor it = m_Covering[level].begin(); it != m_Covering[level].end(); ++it)
      ++parentCellCounts[it->Parent()];

    vector<CellId> parentCells, childCells;
    for (ConstIteartor it = m_Covering[level].begin(); it != m_Covering[level].end(); ++it)
    {
      if (parentCellCounts[it->Parent()] > 1)
        parentCells.push_back(it->Parent());
      else
        childCells.push_back(*it);
    }
    ASSERT(IsSorted(parentCells.begin(), parentCells.end(), LessLevelOrder()), (parentCells));
    ASSERT(IsSorted(childCells.begin(), childCells.end(), LessLevelOrder()), (childCells));
    m_Covering[level].swap(childCells);
    parentCells.erase(unique(parentCells.begin(), parentCells.end()), parentCells.end());
    AppendToVector(m_Covering[level - 1], parentCells);
  }

  static void AppendToVector(vector<CellId> & a, vector<CellId> const & b)
  {
    ASSERT(IsSortedAndUnique(a.begin(), a.end(), LessLevelOrder()), (a));
    ASSERT(IsSortedAndUnique(b.begin(), b.end(), LessLevelOrder()), (b));
    vector<CellId> merged;
    set_union(a.begin(), a.end(), b.begin(), b.end(), back_inserter(merged), LessLevelOrder());
    a.swap(merged);
  }

  template <typename CompareT>
  struct CompareCellsAtLevel
  {
    explicit CompareCellsAtLevel(int level) : m_Level(level) {}

    bool operator()(CellId id1, CellId id2) const
    {
      return m_Comp(id1.AncestorAtLevel(m_Level), id2.AncestorAtLevel(m_Level));
    }

    CompareT m_Comp;
    int m_Level;
  };

  void AppendWithoutNormalize(Covering const & c)
  {
    for (int level = CellId::DEPTH_LEVELS - 1; level >= 0; --level)
      AppendToVector(m_Covering[level], c.m_Covering[level]);
  }

  void Sort()
  {
    for (int level = 0; level < CellId::DEPTH_LEVELS; ++level)
      sort(m_Covering[level].begin(), m_Covering[level].end(), LessLevelOrder());
  }

  void Unique()
  {
    for (int level = 0; level < CellId::DEPTH_LEVELS; ++level)
    {
      vector<CellId> & covering = m_Covering[level];
      ASSERT(IsSorted(covering.begin(), covering.end(), LessLevelOrder()), (covering));
      covering.erase(unique(covering.begin(), covering.end()), covering.end());
    }
  }

  void RemoveDuplicateChildren()
  {
    RemoveDuplicateChildrenImpl();
#ifdef DEBUG
    // Assert that all duplicate children were removed.
    vector<CellId> v1, v2;
    OutputToVector(v1);
    RemoveDuplicateChildrenImpl();
    OutputToVector(v2);
    ASSERT_EQUAL(v1, v2, ());
#endif
  }

  void RemoveDuplicateChildrenImpl()
  {
    for (int parentLevel = 0; parentLevel < static_cast<int>(m_Covering.size()) - 1; ++parentLevel)
    {
      if (m_Covering[parentLevel].empty())
        continue;
      for (int childLevel = parentLevel + 1; childLevel < static_cast<int>(m_Covering.size());
           ++childLevel)
      {
        vector<CellId> substracted;
        CompareCellsAtLevel<LessLevelOrder> comparator(parentLevel);
        ASSERT(IsSorted(m_Covering[childLevel].begin(), m_Covering[childLevel].end(), comparator),
               (m_Covering[childLevel]));
        ASSERT(IsSorted(m_Covering[parentLevel].begin(), m_Covering[parentLevel].end(), comparator),
               (m_Covering[parentLevel]));
        SetDifferenceUnlimited(m_Covering[childLevel].begin(), m_Covering[childLevel].end(),
                               m_Covering[parentLevel].begin(), m_Covering[parentLevel].end(),
                               back_inserter(substracted), comparator);
        m_Covering[childLevel].swap(substracted);
      }
    }
  }

  void RemoveFullSquares()
  {
    vector<CellId> cellsToAppend;
    for (int level = static_cast<int>(m_Covering.size()) - 1; level >= 0; --level)
    {
      // a -> b + parents
      vector<CellId> const & a = m_Covering[level];
      vector<CellId> b;
      vector<CellId> parents;
      b.reserve(a.size());
      for (size_t i = 0; i < a.size(); ++i)
      {
        if (i + 3 < a.size())
        {
          CellId const parent = a[i].Parent();
          if (parent == a[i + 1].Parent() && parent == a[i + 2].Parent() &&
              parent == a[i + 3].Parent())
          {
            parents.push_back(parent);
            i += 3;
            continue;
          }
        }
        b.push_back(a[i]);
      }
      m_Covering[level].swap(b);
      if (level > 0)
        AppendToVector(m_Covering[level - 1], parents);
    }
  }

  size_t CalculateSize() const
  {
    size_t size = 0;
    for (int level = 0; level < CellId::DEPTH_LEVELS; ++level)
      size += m_Covering[level].size();
    return size;
  }

  struct CoverTriangleInfo
  {
    m2::PointD m_A, m_B, m_C;
    int m_Level;
    Covering<CellId> * m_pCovering;
  };

  static void CoverTriangleImpl(CoverTriangleInfo const & info, CellId const cell)
  {
    ASSERT_LESS_OR_EQUAL(cell.Level(), info.m_Level, (info.m_A, info.m_B, info.m_C));
    CellObjectIntersection intersection =
        IntersectCellWithTriangle(cell, info.m_A, info.m_B, info.m_C);

    if (intersection == CELL_OBJECT_NO_INTERSECTION)
      return;

    if (cell.Level() == info.m_Level || intersection == CELL_INSIDE_OBJECT)
    {
      info.m_pCovering->m_Covering[cell.Level()].push_back(cell);
      return;
    }

    for (uint8_t child = 0; child < 4; ++child)
      CoverTriangleImpl(info, cell.Child(child));
  }

  array<vector<CellId>, CellId::DEPTH_LEVELS> m_Covering;  // Covering by level.
  size_t m_Size;
};
}
