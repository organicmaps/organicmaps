#pragma once
#include "../geometry/point2d.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/logging.hpp"
#include "../base/set_operations.hpp"
#include "../base/stl_add.hpp"
#include "../std/algorithm.hpp"
#include "../std/array.hpp"
#include "../std/functional.hpp"
#include "../std/iterator.hpp"
#include "../std/map.hpp"
#include "../std/queue.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

namespace covering
{

enum CellTriangleIntersection
{
  CELL_TRIANGLE_NO_INTERSECTION = 0,
  CELL_TRIANGLE_INTERSECT = 1,
  CELL_INSIDE_TRIANGLE = 2,
  TRIANGLE_INSIDE_CELL = 3
};

template <class CellIdT>
CellTriangleIntersection IntersectCellWithTriangle(
    CellIdT const cell, m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
{
  pair<uint32_t, uint32_t> const xy = cell.XY();
  uint32_t const r = cell.Radius();
  m2::PointD const cellCorners[4] =
  {
    m2::PointD(xy.first - r, xy.second - r),
    m2::PointD(xy.first - r, xy.second + r),
    m2::PointD(xy.first + r, xy.second + r),
    m2::PointD(xy.first + r, xy.second - r)
  };
  for (int i = 0; i < 4; ++i)
  {
    m2::PointD const & p1 = cellCorners[i];
    m2::PointD const & p2 = cellCorners[i == 0 ? 3 : i - 1];
    if (m2::SegmentsIntersect(a, b, p1, p2)) return CELL_TRIANGLE_INTERSECT;
    if (m2::SegmentsIntersect(b, c, p1, p2)) return CELL_TRIANGLE_INTERSECT;
    if (m2::SegmentsIntersect(c, a, p1, p2)) return CELL_TRIANGLE_INTERSECT;
  }
  if (xy.first - r <= a.x && a.x <= xy.first + r && xy.second - r <= a.y && a.y <= xy.second + r)
    return TRIANGLE_INSIDE_CELL;
  if (m2::IsPointStrictlyInsideTriangle(cellCorners[0], a, b, c))
    return CELL_INSIDE_TRIANGLE;
  return CELL_TRIANGLE_NO_INTERSECTION;
}

template <class CellIdT, typename F>
void CoverTriangle(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c,
                   F const & output, uint8_t const level = CellIdT::DEPTH_LEVELS - 1,
                   CellIdT const startCell = CellIdT::Root())
{
  ASSERT_LESS_OR_EQUAL(startCell.Level(), level, (a, b, c));
  CellTriangleIntersection intersection = IntersectCellWithTriangle(startCell, a, b, c);

  if (intersection == CELL_TRIANGLE_NO_INTERSECTION)
    return;

  if (startCell.Level() == level || intersection == CELL_INSIDE_TRIANGLE)
  {
    output(startCell);
    return;
  }

  for (uint8_t child = 0; child < 4; ++child)
    CoverTriangle(a, b, c, output, level, startCell.Child(child));
}

template <class CellIdT>
class Covering
{
public:
  typedef CellIdT CellId;
  typedef typename CellId::LessQueueOrder LessQueueOrder;

  Covering() : m_Size(0) {}

  explicit Covering(CellId cell) : m_Size(1)
  {
    m_Covering[cell.Level()].push_back(cell);
  }

  explicit Covering(vector<CellId> const & v, int64_t minId = 0)
  {
    for (size_t i = 0; i < v.size(); ++i)
      m_Covering[v[i].Level()].push_back(v[i]);
    Sort();
    Unique();
    RemoveDuplicateChildren();
    RemoveFullSquares(minId);
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

  void OutputToVector(vector<int64_t> & result) const
  {
    for (int level = 0; level < CellId::DEPTH_LEVELS; ++level)
      for (size_t i = 0; i < m_Covering[level].size(); ++i)
        result.push_back(m_Covering[level][i].ToInt64());
  }

  void Simplify(int64_t minId = 0)
  {
    int cellsSimplified = 0;
    int const initialSize = m_Size;
    for (int level = CellId::DEPTH_LEVELS - 1; level > 1; --level)
    {
      if (m_Covering[level].size() >= 2)
      {
        int const initialLevelSize = static_cast<int>(m_Covering[level].size());
        SimplifyLevel(level, minId);
        cellsSimplified += initialLevelSize - static_cast<int>(m_Covering[level].size());
        if (cellsSimplified > initialSize / 2)
          break;
      }
    }
    RemoveDuplicateChildren();
    RemoveFullSquares(minId);
    m_Size = CalculateSize();
  }

private:

  void SimplifyLevel(int level, int64_t minId)
  {
    map<CellId, uint32_t, LessQueueOrder> parentCellCounts;
    typedef typename vector<CellId>::const_iterator ConstIteartor;
    for (ConstIteartor it = m_Covering[level].begin(); it != m_Covering[level].end(); ++it)
      if (it->Parent().ToInt64() >= minId)
        ++parentCellCounts[it->Parent()];

    vector<CellId> parentCells, childCells;
    for (ConstIteartor it = m_Covering[level].begin(); it != m_Covering[level].end(); ++it)
    {
      if (parentCellCounts[it->Parent()] > 1)
        parentCells.push_back(it->Parent());
      else
        childCells.push_back(*it);
    }
    ASSERT(IsSorted(parentCells.begin(), parentCells.end(), LessQueueOrder()), (parentCells));
    ASSERT(IsSorted(childCells.begin(), childCells.end(), LessQueueOrder()), (childCells));
    m_Covering[level].swap(childCells);
    parentCells.erase(unique(parentCells.begin(), parentCells.end()), parentCells.end());
    AppendToVector(m_Covering[level - 1], parentCells);
  }

  static void AppendToVector(vector<CellId> & a, vector<CellId> const & b)
  {
    ASSERT(IsSortedAndUnique(a.begin(), a.end(), LessQueueOrder()), (a));
    ASSERT(IsSortedAndUnique(b.begin(), b.end(), LessQueueOrder()), (b));
    vector<CellId> merged;
    set_union(a.begin(), a.end(), b.begin(), b.end(), back_inserter(merged), LessQueueOrder());
    a.swap(merged);
  }

  template <typename CompareT>
  struct CompareCellsAtLevel
  {
    explicit CompareCellsAtLevel(int level) : m_Level(level) {}

    bool operator() (CellId id1, CellId id2) const
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
      sort(m_Covering[level].begin(), m_Covering[level].end(), LessQueueOrder());
  }

  void Unique()
  {
    for (int level = 0; level < CellId::DEPTH_LEVELS; ++level)
    {
      vector<CellId> & covering = m_Covering[level];
      ASSERT(IsSorted(covering.begin(), covering.end(), LessQueueOrder()), (covering));
      covering.erase(unique(covering.begin(), covering.end()), covering.end());
    }
  }

  void RemoveDuplicateChildren()
  {
    RemoveDuplicateChildrenImpl();
#ifdef DEBUG
    // Assert that all duplicate children were removed.
    vector<int64_t> v1, v2;
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
        CompareCellsAtLevel<LessQueueOrder> comparator(parentLevel);
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

  void RemoveFullSquares(int64_t minId = 0)
  {
    vector<CellId> cellsToAppend;
    for (int level = m_Covering.size() - 1; level >= 0; --level)
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
          if (parent == a[i+1].Parent() &&
              parent == a[i+2].Parent() &&
              parent == a[i+3].Parent() &&
              parent.ToInt64() >= minId)
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
    CellTriangleIntersection intersection =
        IntersectCellWithTriangle(cell, info.m_A, info.m_B, info.m_C);

    if (intersection == CELL_TRIANGLE_NO_INTERSECTION)
      return;

    if (cell.Level() == info.m_Level || intersection == CELL_INSIDE_TRIANGLE)
    {
      info.m_pCovering->m_Covering[cell.Level()].push_back(cell);
      return;
    }

    for (uint8_t child = 0; child < 4; ++child)
      CoverTriangleImpl(info, cell.Child(child));
  }


  array<vector<CellId>, CellId::DEPTH_LEVELS> m_Covering; // Covering by level.
  size_t m_Size;
};


}
