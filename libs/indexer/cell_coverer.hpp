#pragma once

#include "indexer/cell_id.hpp"

#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include <array>
#include <cstdint>
#include <queue>
#include <utility>
#include <vector>

// TODO: Move neccessary functions to geometry/covering_utils.hpp and delete this file.

constexpr int SPLIT_RECT_CELLS_COUNT = 512;

template <typename Bounds, typename CellId>
inline size_t SplitRectCell(CellId const & id, m2::RectD const & rect,
                            std::array<std::pair<CellId, m2::RectD>, 4> & result)
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

// Covers |rect| with at most |cellsCount| cells that have levels equal to or less than |maxLevel|.
template <typename Bounds, typename CellId>
inline void CoverRect(m2::RectD rect, size_t cellsCount, int maxLevel, std::vector<CellId> & result)
{
  ASSERT(result.empty(), ());
  {
    // Cut rect with world bound coordinates.
    if (!rect.Intersect(Bounds::FullRect()))
      return;
    ASSERT(rect.IsValid(), ());
  }

  auto const commonCell =
      CellIdConverter<Bounds, CellId>::Cover2PointsWithCell(rect.minX(), rect.minY(), rect.maxX(), rect.maxY());

  std::priority_queue<CellId, buffer_vector<CellId, SPLIT_RECT_CELLS_COUNT>, typename CellId::GreaterLevelOrder>
      cellQueue;
  cellQueue.push(commonCell);

  CHECK_GREATER_OR_EQUAL(maxLevel, 0, ());
  while (!cellQueue.empty() && cellQueue.size() + result.size() < cellsCount)
  {
    auto id = cellQueue.top();
    cellQueue.pop();

    while (id.Level() > maxLevel)
      id = id.Parent();

    if (id.Level() == maxLevel)
    {
      result.push_back(id);
      break;
    }

    std::array<std::pair<CellId, m2::RectD>, 4> arr;
    size_t const count = SplitRectCell<Bounds>(id, rect, arr);

    if (cellQueue.size() + result.size() + count <= cellsCount)
    {
      for (size_t i = 0; i < count; ++i)
        if (rect.IsRectInside(arr[i].second))
          result.push_back(arr[i].first);
        else
          cellQueue.push(arr[i].first);
    }
    else
    {
      result.push_back(id);
    }
  }

  for (; !cellQueue.empty(); cellQueue.pop())
  {
    auto id = cellQueue.top();
    while (id.Level() < maxLevel)
    {
      std::array<std::pair<CellId, m2::RectD>, 4> arr;
      size_t const count = SplitRectCell<Bounds>(id, rect, arr);
      ASSERT_GREATER(count, 0, ());
      if (count > 1)
        break;
      id = arr[0].first;
    }
    result.push_back(id);
  }
}

// Covers |rect| with cells using spiral order starting from the rect center cell of |maxLevel|.
template <typename Bounds, typename CellId>
void CoverSpiral(m2::RectD rect, int maxLevel, std::vector<CellId> & result)
{
  using Converter = CellIdConverter<Bounds, CellId>;

  enum class Direction : uint8_t
  {
    Right = 0,
    Down = 1,
    Left = 2,
    Up = 3
  };

  CHECK(result.empty(), ());
  // Cut rect with world bound coordinates.
  if (!rect.Intersect(Bounds::FullRect()))
    return;
  CHECK(rect.IsValid(), ());

  CHECK_GREATER_OR_EQUAL(maxLevel, 0, ());
  auto centralCell = Converter::ToCellId(rect.Center().x, rect.Center().y);
  while (maxLevel < centralCell.Level())
    centralCell = centralCell.Parent();

  result.push_back(centralCell);

  // Area around CentralCell will be covered with surrounding cells.
  //
  //          * -> * -> * -> *
  //          ^              |
  //          |              V
  //          *    C -> *    *
  //          ^         |    |
  //          |         V    V
  //          * <- * <- *    *
  //
  // To get the best ranking quality we should use the smallest cell size but it's not
  // efficient because it generates too many index requests. To get good quality-performance
  // tradeoff we cover area with |maxCount| small cells, then increase cell size and cover
  // area with |maxCount| bigger cells. We increase cell size until |rect| is covered.
  // We start covering from the center each time and it's ok for ranking because each object
  // appears in result at most once.
  // |maxCount| may be adjusted after testing to ensure better quality-performance tradeoff.
  uint32_t constexpr maxCount = 64;

  auto const nextDirection = [](Direction direction)
  { return static_cast<Direction>((static_cast<uint8_t>(direction) + 1) % 4); };

  auto const nextCoords = [](std::pair<int32_t, int32_t> const & xy, Direction direction, uint32_t step)
  {
    auto res = xy;
    switch (direction)
    {
    case Direction::Right: res.first += step; break;
    case Direction::Down: res.second -= step; break;
    case Direction::Left: res.first -= step; break;
    case Direction::Up: res.second += step; break;
    }
    return res;
  };

  auto const coordsAreValid = [](std::pair<int32_t, int32_t> const & xy)
  {
    return xy.first >= 0 && xy.second >= 0 && static_cast<decltype(CellId::MAX_COORD)>(xy.first) <= CellId::MAX_COORD &&
           static_cast<decltype(CellId::MAX_COORD)>(xy.second) <= CellId::MAX_COORD;
  };

  m2::RectD coveredRect;
  static_assert(CellId::MAX_COORD == static_cast<int32_t>(CellId::MAX_COORD), "");
  while (centralCell.Level() > 0 && !coveredRect.IsRectInside(rect))
  {
    uint32_t count = 0;
    auto const centerXY = centralCell.XY();
    // We support negative coordinates while covering and check coordinates validity before pushing
    // cell to |result|.
    std::pair<int32_t, int32_t> xy{centerXY.first, centerXY.second};
    auto direction = Direction::Right;
    int sideLength = 1;
    // Indicates whether it is the first pass with current |sideLength|. We use spiral cells order and
    // must increment |sideLength| every second side pass. |sideLength| and |direction| will behave like:
    // 1 right, 1 down, 2 left, 2 up, 3 right, 3 down, etc.
    bool evenPass = true;
    while (count <= maxCount && !coveredRect.IsRectInside(rect))
    {
      for (int i = 0; i < sideLength; ++i)
      {
        xy = nextCoords(xy, direction, centralCell.Radius() * 2);
        if (coordsAreValid(xy))
        {
          auto const cell = CellId::FromXY(xy.first, xy.second, centralCell.Level());
          double minCellX, minCellY, maxCellX, maxCellY;
          Converter::GetCellBounds(cell, minCellX, minCellY, maxCellX, maxCellY);
          auto const cellRect = m2::RectD(minCellX, minCellY, maxCellX, maxCellY);
          coveredRect.Add(cellRect);

          if (rect.IsIntersect(cellRect))
            result.push_back(cell);
        }
        ++count;
      }

      if (!evenPass)
        ++sideLength;

      direction = nextDirection(direction);
      evenPass = !evenPass;
    }
    centralCell = centralCell.Parent();
  }
}
