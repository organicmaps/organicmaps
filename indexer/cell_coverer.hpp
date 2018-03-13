#pragma once

#include "indexer/cell_id.hpp"

#include "base/buffer_vector.hpp"

#include "std/queue.hpp"
#include "std/vector.hpp"

// TODO: Move neccessary functions to geometry/covering_utils.hpp and delete this file.

constexpr int SPLIT_RECT_CELLS_COUNT = 512;

template <typename Bounds, typename CellId>
inline size_t SplitRectCell(CellId const & id, m2::RectD const & rect,
                            array<pair<CellId, m2::RectD>, 4> & result)
{
  size_t index = 0;
  for (int8_t i = 0; i < 4; ++i)
  {
    auto const child = id.Child(i);
    double minCellX, minCellY, maxCellX, maxCellY;
    CellIdConverter<Bounds, CellId>::GetCellBounds(child, minCellX, minCellY, maxCellX, maxCellY);

    m2::RectD const childRect(minCellX, minCellY, maxCellX, maxCellY);
    if (rect.IsIntersect(childRect))
      result[index++] = {child, childRect};
  }
  return index;
}

template <typename Bounds, typename CellId>
inline void CoverRect(m2::RectD rect, size_t cellsCount, int maxDepth, vector<CellId> & result)
{
  ASSERT(result.empty(), ());
  {
    // Cut rect with world bound coordinates.
    if (!rect.Intersect(Bounds::FullRect()))
      return;
    ASSERT(rect.IsValid(), ());
  }

  auto const commonCell = CellIdConverter<Bounds, CellId>::Cover2PointsWithCell(
      rect.minX(), rect.minY(), rect.maxX(), rect.maxY());

  priority_queue<CellId, buffer_vector<CellId, SPLIT_RECT_CELLS_COUNT>,
                 typename CellId::GreaterLevelOrder>
      cellQueue;
  cellQueue.push(commonCell);

  maxDepth -= 1;

  while (!cellQueue.empty() && cellQueue.size() + result.size() < cellsCount)
  {
    auto id = cellQueue.top();
    cellQueue.pop();

    while (id.Level() > maxDepth)
      id = id.Parent();

    if (id.Level() == maxDepth)
    {
      result.push_back(id);
      break;
    }

    array<pair<CellId, m2::RectD>, 4> arr;
    size_t const count = SplitRectCell<Bounds>(id, rect, arr);

    if (cellQueue.size() + result.size() + count <= cellsCount)
    {
      for (size_t i = 0; i < count; ++i)
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
    auto id = cellQueue.top();
    while (id.Level() < maxDepth)
    {
      array<pair<CellId, m2::RectD>, 4> arr;
      size_t const count = SplitRectCell<Bounds>(id, rect, arr);
      ASSERT_GREATER(count, 0, ());
      if (count > 1)
        break;
      id = arr[0].first;
    }
    result.push_back(id);
  }
}
