#pragma once

#include "indexer/cell_id.hpp"

#include "std/queue.hpp"
#include "std/vector.hpp"

// TODO: Move neccessary functions to geometry/covering_utils.hpp and delete this file.

template <typename BoundsT, typename CellIdT>
inline void SplitRectCell(CellIdT id,
                          double minX, double minY,
                          double maxX, double maxY,
                          vector<CellIdT> & result)
{
  for (int8_t i = 0; i < 4; ++i)
  {
    CellIdT child = id.Child(i);
    double minCellX, minCellY, maxCellX, maxCellY;
    CellIdConverter<BoundsT, CellIdT>::GetCellBounds(child, minCellX, minCellY, maxCellX, maxCellY);
    if (!((maxX < minCellX) || (minX > maxCellX) || (maxY < minCellY) || (minY > maxCellY)))
      result.push_back(child);
  }
}

template <typename BoundsT, typename CellIdT>
inline void CoverRect(double minX, double minY,
                      double maxX, double maxY,
                      size_t cells_count, int maxDepth,
                      vector<CellIdT> & cells)
{
  ASSERT_LESS(minX, maxX, ());
  ASSERT_LESS(minY, maxY, ());

  if (minX < BoundsT::minX) minX = BoundsT::minX;
  if (minY < BoundsT::minY) minY = BoundsT::minY;
  if (maxX > BoundsT::maxX) maxX = BoundsT::maxX;
  if (maxY > BoundsT::maxY) maxY = BoundsT::maxY;

  if (minX >= maxX || minY >= maxY)
    return;

  CellIdT commonCell =
      CellIdConverter<BoundsT, CellIdT>::Cover2PointsWithCell(minX, minY, maxX, maxY);

  vector<CellIdT> result;

  queue<CellIdT> cellQueue;
  cellQueue.push(commonCell);

  maxDepth -= 1;

  while (!cellQueue.empty() && cellQueue.size() + result.size() < cells_count)
  {
    CellIdT id = cellQueue.front();
    cellQueue.pop();

    while (id.Level() > maxDepth)
      id = id.Parent();

    if (id.Level() == maxDepth)
    {
      result.push_back(id);
      break;
    }

    vector<CellIdT> children;
    SplitRectCell<BoundsT>(id, minX, minY, maxX, maxY, children);

    // Children shouldn't be empty, but if it is, ignore this cellid in release.
    ASSERT(!children.empty(), (id, minX, minY, maxX, maxY));
    if (children.empty())
    {
      result.push_back(id);
      continue;
    }

    if (cellQueue.size() + result.size() + children.size() <= cells_count)
    {
      for (size_t i = 0; i < children.size(); ++i)
        cellQueue.push(children[i]);
    }
    else
      result.push_back(id);
  }

  for (; !cellQueue.empty(); cellQueue.pop())
    result.push_back(cellQueue.front());

  for (size_t i = 0; i < result.size(); ++i)
  {
    CellIdT id = result[i];
    while (id.Level() < maxDepth)
    {
      vector<CellIdT> children;
      SplitRectCell<BoundsT>(id, minX, minY, maxX, maxY, children);
      if (children.size() == 1)
        id = children[0];
      else
        break;
    }
    result[i] = id;
  }

  ASSERT_LESS_OR_EQUAL(result.size(), cells_count, (minX, minY, maxX, maxY));
  cells.insert(cells.end(), result.begin(), result.end());
}
