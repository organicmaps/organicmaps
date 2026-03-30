#include "testing/testing.hpp"

#include "geometry/spatial_hash_grid.hpp"

#include <set>

UNIT_TEST(SpatialHashGrid_ToCell)
{
  m2::SpatialHashGrid grid(1.0);

  // Points in the same unit cell.
  TEST_EQUAL(grid.ToCell({0.5, 0.5}), grid.ToCell({0.9, 0.1}), ());

  // Points in different cells.
  TEST(!(grid.ToCell({0.5, 0.5}) == grid.ToCell({1.5, 0.5})), ());

  // Negative coordinates.
  auto const c = grid.ToCell({-0.1, -0.1});
  TEST_EQUAL(c.x, -1, ());
  TEST_EQUAL(c.y, -1, ());
}

UNIT_TEST(SpatialHashGrid_ToCellXY)
{
  m2::SpatialHashGrid grid(0.5);
  TEST_EQUAL(grid.ToCell(1.2, 3.4), grid.ToCell({1.2, 3.4}), ());
}

UNIT_TEST(SpatialHashGrid_SmallCellSize)
{
  m2::SpatialHashGrid grid(1e-5);  // ~1 meter in mercator

  m2::PointD const a(10.00001, 20.00002);
  m2::PointD const b(10.00001, 20.00002);  // same
  m2::PointD const c(10.00003, 20.00002);  // different cell

  TEST_EQUAL(grid.ToCell(a), grid.ToCell(b), ());
  TEST(!(grid.ToCell(a) == grid.ToCell(c)), ());
}

UNIT_TEST(SpatialHashGrid_GetNearbyCells_Returns9)
{
  m2::SpatialHashGrid grid(1.0);
  auto const cells = grid.GetNearbyCells({5.5, 5.5});
  TEST_EQUAL(cells.size(), 9, ());

  // All 9 cells should be unique.
  std::set<std::pair<int64_t, int64_t>> unique;
  for (auto const & c : cells)
    unique.emplace(c.x, c.y);
  TEST_EQUAL(unique.size(), 9, ());
}

// Two epsilon-close points on a cell boundary are guaranteed to share
// at least one cell in their 3x3 neighborhoods.
UNIT_TEST(SpatialHashGrid_NearbyCells_BoundaryOverlap)
{
  double const eps = 1e-5;
  m2::SpatialHashGrid grid(eps);

  // Points straddling a cell boundary: just below and just above x = 1.0.
  m2::PointD const a(1.0 - eps * 0.1, 5.0);
  m2::PointD const b(1.0 + eps * 0.1, 5.0);

  // They may be in different cells...
  // ...but their 3x3 neighborhoods must overlap.
  auto const cellsA = grid.GetNearbyCells(a);
  auto const cellsB = grid.GetNearbyCells(b);

  std::set<std::pair<int64_t, int64_t>> setA, setB;
  for (auto const & c : cellsA)
    setA.emplace(c.x, c.y);
  for (auto const & c : cellsB)
    setB.emplace(c.x, c.y);

  bool overlap = false;
  for (auto const & c : setA)
  {
    if (setB.count(c))
    {
      overlap = true;
      break;
    }
  }
  TEST(overlap, ("3x3 neighborhoods of epsilon-close points must overlap"));
}

UNIT_TEST(SpatialHashGrid_Map)
{
  m2::SpatialHashGrid grid(1.0);
  m2::SpatialHashGrid::Map<int> map;

  map[grid.ToCell({1.5, 2.5})] = 42;
  map[grid.ToCell({3.5, 4.5})] = 99;

  TEST_EQUAL(map.size(), 2, ());
  TEST_EQUAL(map[grid.ToCell({1.1, 2.9})], 42, ());  // same cell as (1.5, 2.5)
}

// Practical pattern: insert points into grid, then find neighbors via 3x3 lookup.
UNIT_TEST(SpatialHashGrid_PointProximityQuery)
{
  double const eps = 0.5;
  m2::SpatialHashGrid grid(eps);
  m2::SpatialHashGrid::Map<std::vector<size_t>> index;

  std::vector<m2::PointD> points = {{1.0, 1.0}, {1.3, 1.1}, {5.0, 5.0}, {1.0, 1.4}};
  for (size_t i = 0; i < points.size(); ++i)
    index[grid.ToCell(points[i])].push_back(i);

  // Query neighbors of (1.2, 1.2) — should find points 0, 1, 3 (all within ~0.4).
  std::vector<size_t> found;
  for (auto const & cell : grid.GetNearbyCells({1.2, 1.2}))
  {
    auto it = index.find(cell);
    if (it != index.end())
      found.insert(found.end(), it->second.begin(), it->second.end());
  }

  // Should find at least points 0, 1, 3 (not point 2 at (5,5)).
  std::set<size_t> foundSet(found.begin(), found.end());
  TEST(foundSet.count(0), ());
  TEST(foundSet.count(1), ());
  TEST(foundSet.count(3), ());
  TEST(!foundSet.count(2), ("Point at (5,5) should not be found"));
}
