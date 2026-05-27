#pragma once

#include "geometry/point2d.hpp"

#include <array>
#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace m2
{
/// Floor-based spatial grid cell for epsilon-proximity queries on m2::PointD.
///
/// No single floor-based hash guarantees hash(a)==hash(b) for all epsilon-equal points
/// (two close points can straddle a cell boundary), so lookups must always probe
/// the 3x3 neighborhood via GetNearbyCells().
///
/// Usage:
///   SpatialHashGrid grid(cellSize);
///   auto cell = grid.ToCell(point);
///   for (auto const & c : grid.GetNearbyCells(point)) { ... }
///
/// As an unordered_map key:
///   std::unordered_map<SpatialHashGrid::Cell, T, SpatialHashGrid::Hash> map;
class SpatialHashGrid
{
  struct Cell
  {
    int64_t x, y;
    bool operator==(Cell const &) const = default;

    friend std::string DebugPrint(Cell const & c)
    {
      return "Cell(" + std::to_string(c.x) + ", " + std::to_string(c.y) + ")";
    }
  };

  struct Hash
  {
    size_t operator()(Cell const & c) const
    {
      size_t seed = std::hash<int64_t>{}(c.x);
      seed ^= std::hash<int64_t>{}(c.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      return seed;
    }
  };

public:
  template <class T>
  using Map = std::unordered_map<Cell, T, Hash>;

  explicit SpatialHashGrid(double cellSize) : m_cellSize(cellSize) {}

  Cell ToCell(PointD const & p) const { return ToCell(p.x, p.y); }

  Cell ToCell(double x, double y) const
  {
    return {static_cast<int64_t>(std::floor(x / m_cellSize)), static_cast<int64_t>(std::floor(y / m_cellSize))};
  }

  /// Returns 3x3 neighborhood of cells that may contain points within cellSize of @p p.
  std::array<Cell, 9> GetNearbyCells(PointD const & p) const
  {
    auto const c = ToCell(p);
    return {{
        {c.x - 1, c.y - 1},
        {c.x, c.y - 1},
        {c.x + 1, c.y - 1},
        {c.x - 1, c.y},
        c,
        {c.x + 1, c.y},
        {c.x - 1, c.y + 1},
        {c.x, c.y + 1},
        {c.x + 1, c.y + 1},
    }};
  }

  bool IsEqual(m2::PointD const & p1, m2::PointD const & p2) const { return p1.EqualDxDy(p2, m_cellSize); }

private:
  double m_cellSize;
};

template <class T>
class PointHashMap
{
public:
  explicit PointHashMap(double cellSize) : m_grid(cellSize) {}

  template <class... Args>
  void Emplace(PointD const & pt, Args &&... args)
  {
    m_points[m_grid.ToCell(pt)].emplace_back(std::piecewise_construct, std::forward_as_tuple(pt),
                                             std::forward_as_tuple(std::forward<Args>(args)...));
  }

  /// Calls @p fn for values whose stored points are in nearby cells and equal to @p pt.
  template <class Fn>
  void ForEachPoint(PointD const & pt, Fn && fn) const
  {
    for (auto const & cell : m_grid.GetNearbyCells(pt))
    {
      auto const it = m_points.find(cell);
      if (it == m_points.end())
        continue;

      for (auto const & [storedPoint, value] : it->second)
        if (m_grid.IsEqual(storedPoint, pt))
          fn(value);
    }
  }

private:
  using Bucket = std::vector<std::pair<PointD, T>>;

  SpatialHashGrid m_grid;
  SpatialHashGrid::Map<Bucket> m_points;
};
}  // namespace m2
