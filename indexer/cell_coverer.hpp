#pragma once

#include "indexer/cell_id.hpp"

#include "base/buffer_vector.hpp"

#include "std/queue.hpp"
#include "std/vector.hpp"

// TODO: Move neccessary functions to geometry/covering_utils.hpp and delete this file.

constexpr int SPLIT_RECT_CELLS_COUNT = 512;

template <typename BoundsT, typename CellIdT>
inline int SplitRectCell(CellIdT const & id, m2::RectD const & rect,
                         array<pair<CellIdT, m2::RectD>, 4> & result)
{
  int index = 0;
  for (int8_t i = 0; i < 4; ++i)
  {
    CellIdT const child = id.Child(i);
    double minCellX, minCellY, maxCellX, maxCellY;
    CellIdConverter<BoundsT, CellIdT>::GetCellBounds(child, minCellX, minCellY, maxCellX, maxCellY);

    m2::RectD const childRect(minCellX, minCellY, maxCellX, maxCellY);
    if (rect.IsIntersect(childRect))
      result[index++] = {child, childRect};
  }
  return index;
}

template <typename BoundsT, typename CellIdT>
inline void CoverRect(m2::RectD rect, size_t cellsCount, int maxDepth,
                      vector<CellIdT> & result)
{
  ASSERT(result.empty(), ());
  {
    // Fit rect to converter bounds.
    if (!rect.Intersect({ BoundsT::minX, BoundsT::minY, BoundsT::maxX, BoundsT::maxY }))
      return;
    ASSERT(rect.IsValid(), ());
  }

  CellIdT const commonCell = CellIdConverter<BoundsT, CellIdT>::Cover2PointsWithCell(
        rect.minX(), rect.minY(), rect.maxX(), rect.maxY());

  priority_queue<CellIdT,
                 buffer_vector<CellIdT, SPLIT_RECT_CELLS_COUNT>,
                 typename CellIdT::GreaterLevelOrder> cellQueue;
  cellQueue.push(commonCell);

  maxDepth -= 1;

  while (!cellQueue.empty() && cellQueue.size() + result.size() < cellsCount)
  {
    CellIdT id = cellQueue.top();
    cellQueue.pop();

    while (id.Level() > maxDepth)
      id = id.Parent();

    if (id.Level() == maxDepth)
    {
      result.push_back(id);
      break;
    }

    array<pair<CellIdT, m2::RectD>, 4> arr;
    int const count = SplitRectCell<BoundsT>(id, rect, arr);

    if (cellQueue.size() + result.size() + count <= cellsCount)
    {
      for (int i = 0; i < count; ++i)
      {
        if (rect.IsRectInside(arr[i].second))
          result.push_back(arr[i].first);
        else
          cellQueue.push(arr[i].first);
      }
    }
    else
    {
      result.push_back(id);
    }
  }

  for (; !cellQueue.empty(); cellQueue.pop())
  {
    CellIdT id = cellQueue.top();
    while (id.Level() < maxDepth)
    {
      array<pair<CellIdT, m2::RectD>, 4> arr;
      int const count = SplitRectCell<BoundsT>(id, rect, arr);
      if (count == 1)
        id = arr[0].first;
      else
        break;
    }
    result.push_back(id);
  }
}
