#include "testing/testing.hpp"

#include "geometry/spatial_hash_grid.hpp"

#include <set>

namespace spatial_hash_grid_test
{

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
  double constexpr eps = 1e-5;
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

UNIT_TEST(PointHashMap_ForEachPoint)
{
  double constexpr eps = 0.5;
  m2::PointHashMap<int> map(eps);

  map.Emplace({1.0, 1.0}, 10);
  map.Emplace({1.3, 1.1}, 20);
  map.Emplace({5.0, 5.0}, 50);
  map.Emplace({1.0, 1.4}, 30);

  std::set<int> foundValues;
  map.ForEachPoint({1.2, 1.2}, [&foundValues](int const & value) { foundValues.insert(value); });

  TEST_EQUAL(foundValues.size(), 3, ());
  TEST(foundValues.count(10), ());
  TEST(foundValues.count(20), ());
  TEST(foundValues.count(30), ());
  TEST(!foundValues.count(50), ());
}

UNIT_TEST(PointHashMap_ForEachPoint_FiltersWithEqualFunctor)
{
  double constexpr eps = 1e-5;
  m2::PointHashMap<int> map(eps);

  m2::PointD const query(1.0 + eps * 0.1, 5.0);
  m2::PointD const matching(1.0 - eps * 0.1, 5.0);
  m2::PointD const nearbyButDifferent(1.0 + eps * 1.1, 5.0);

  map.Emplace(matching, 1);
  map.Emplace(nearbyButDifferent, 2);

  size_t foundCount = 0;
  int foundValue = 0;
  map.ForEachPoint(query, [&](int const & value)
  {
    ++foundCount;
    foundValue = value;
  });

  TEST_EQUAL(foundCount, 1, ());
  TEST_EQUAL(foundValue, 1, ());
}

}  // namespace spatial_hash_grid_test
