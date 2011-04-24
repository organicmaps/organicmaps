#pragma once

#include "cell_id.hpp"

#include "../std/queue.hpp"
#include "../std/vector.hpp"

inline bool IntersectsHoriz(CoordT x1, CoordT y1, CoordT x2, CoordT y2,
                            CoordT y, CoordT l, CoordT r)
{
  CoordT d = (y1 - y) * (y2 - y);
  if (d > 0) return false;

  CoordT x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);

  if ((l - x) * (r - x) <= 0) return true; else return false;
}

inline bool IntersectsVert(CoordT x1, CoordT y1, CoordT x2, CoordT y2,
                           CoordT x, CoordT b, CoordT t)
{
  CoordT d = (x1 - x) * (x2 - x);
  if (d > 0) return false;

  CoordT y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);

  if ((b - y) * (t - y) <= 0) return true; else return false;
}

template <typename BoundsT, typename CellIdT>
inline bool CellIntersects(vector<CoordPointT> const & polyLine, CellIdT id)
{
  CoordT minX, minY, maxX, maxY;
  CellIdConverter<BoundsT, CellIdT>::GetCellBounds(id, minX, minY, maxX, maxY);
  CoordPointT minPoint = make_pair(minX, minY);
  CoordPointT maxPoint = make_pair(maxX, maxY);

  for (size_t i = 0; i < polyLine.size() - 1; ++i)
  {
    if (IntersectsHoriz(polyLine[i].first, polyLine[i].second,
                        polyLine[i + 1].first, polyLine[i + 1].second,
                        minPoint.second, minPoint.first, maxPoint.first)) return true;

    if (IntersectsHoriz(polyLine[i].first, polyLine[i].second,
                        polyLine[i + 1].first, polyLine[i + 1].second,
                        maxPoint.second, minPoint.first, maxPoint.first)) return true;

    if (IntersectsVert(polyLine[i].first, polyLine[i].second,
                       polyLine[i + 1].first, polyLine[i + 1].second,
                       minPoint.first, minPoint.second, maxPoint.second)) return true;

    if (IntersectsVert(polyLine[i].first, polyLine[i].second,
                       polyLine[i + 1].first, polyLine[i + 1].second,
                       maxPoint.first, minPoint.second, maxPoint.second)) return true;
  }

  return false;
}

template <typename BoundsT, typename CellIdT>
inline void SplitCell(vector<CoordPointT> const & polyLine, queue<CellIdT> & cellQueue)
{
  CellIdT id = cellQueue.front();
  cellQueue.pop();

  for (size_t i = 0; i < 4; ++i)
  {
    CellIdT child = id.Child(i);

    if (CellIntersects<BoundsT>(polyLine, child))
    {
      cellQueue.push(child);
    }
  }
}

template <typename ItT>
inline bool FindBounds(ItT begin, ItT end,
                       CoordT & minX, CoordT & minY, CoordT & maxX, CoordT & maxY)
{
  if (begin == end) return false;

  minX = begin->first;
  maxX = begin->first;
  minY = begin->second;
  maxY = begin->second;

  for (ItT it = begin; it != end; ++it)
  {
    if (it->first < minX) minX = it->first;
    if (it->first > maxX) maxX = it->first;
    if (it->second < minY) minY = it->second;
    if (it->second > maxY) maxY = it->second;
  }

  return true;
}

template <typename BoundsT, typename CellIdT>
inline CellIdT CoverPoint(CoordPointT const & point)
{
  return CellIdConverter<BoundsT, CellIdT>::ToCellId(point.first, point.second);
}

template <typename BoundsT, typename CellIdT>
inline void CoverPolyLine(vector< CoordPointT > const & polyLine, size_t cellDepth,
                          vector<CellIdT> & cells)
{
  CoordT minX = 0, minY = 0, maxX = 0, maxY = 0;
  FindBounds(polyLine.begin(), polyLine.end(), minX, minY, maxX, maxY);

  CellIdT commonCell =
      CellIdConverter<BoundsT, CellIdT>::Cover2PointsWithCell(minX, minY, maxX, maxY);

  queue<CellIdT> cellQueue;
  cellQueue.push(commonCell);
  while (cellQueue.front().Level() < static_cast<int>(cellDepth)) // cellQueue.size() < cells_count
  {
    SplitCell<BoundsT>(polyLine, cellQueue);
  }

  while (!cellQueue.empty())
  {
    cells.push_back(cellQueue.front());
    cellQueue.pop();
  }
}

template <typename BoundsT, typename CellIdT>
inline void SplitRectCell(CellIdT id,
                          CoordT minX, CoordT minY,
                          CoordT maxX, CoordT maxY,
                          vector<CellIdT> & result)
{
  for (size_t i = 0; i < 4; ++i)
  {
    CellIdT child = id.Child(i);
    CoordT minCellX, minCellY, maxCellX, maxCellY;
    CellIdConverter<BoundsT, CellIdT>::GetCellBounds(child, minCellX, minCellY, maxCellX, maxCellY);
    if (!((maxX < minCellX) || (minX > maxCellX) || (maxY < minCellY) || (minY > maxCellY)))
      result.push_back(child);
  }
}

template <typename BoundsT, typename CellIdT>
inline void CoverRect(CoordT minX, CoordT minY,
                      CoordT maxX, CoordT maxY,
                      size_t cells_count,
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

  while (!cellQueue.empty() && cellQueue.size() + result.size() < cells_count)
  {
    CellIdT id = cellQueue.front();
    cellQueue.pop();

    if (id.Level() == CellIdT::DEPTH_LEVELS - 1)
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
    while (id.Level() < CellIdT::DEPTH_LEVELS - 1)
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
