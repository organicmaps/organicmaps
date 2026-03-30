#pragma once

#include "geometry/point2d.hpp"

#include <array>
#include <cmath>
#include <cstdint>
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
public:
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

  template <class T>
  using Map = std::unordered_map<Cell, T, Hash>;

  explicit SpatialHashGrid(double cellSize) : m_cellSize(cellSize) {}

  Cell ToCell(m2::PointD const & p) const
  {
    return {static_cast<int64_t>(std::floor(p.x / m_cellSize)), static_cast<int64_t>(std::floor(p.y / m_cellSize))};
  }

  Cell ToCell(double x, double y) const
  {
    return {static_cast<int64_t>(std::floor(x / m_cellSize)), static_cast<int64_t>(std::floor(y / m_cellSize))};
  }

  /// Returns 3x3 neighborhood of cells that may contain points within cellSize of @p p.
  std::array<Cell, 9> GetNearbyCells(m2::PointD const & p) const
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

  double GetCellSize() const { return m_cellSize; }

private:
  double m_cellSize;
};
}  // namespace m2
