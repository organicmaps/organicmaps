#include "testing/testing.hpp"

#include "routing/water_geometry.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>

namespace water_geometry_test
{
using namespace routing::water;
using P = m2::PointD;

auto const kNeverCancelled = [] { return false; };

void Rectangle(Geometry & geometry, double left, double bottom, double right, double top)
{
  geometry.m_triangles.push_back({P(left, bottom), P(right, bottom), P(right, top)});
  geometry.m_triangles.push_back({P(left, bottom), P(right, top), P(left, top)});
}

void CheckAccounting(Path const & path)
{
  TEST_EQUAL(path.m_landGaps.size() + 1, path.m_points.size(), ());
  double water = 0.0, land = 0.0;
  for (size_t i = 1; i < path.m_points.size(); ++i)
    (path.m_landGaps[i - 1] ? land : water) += mercator::DistanceOnEarth(path.m_points[i - 1], path.m_points[i]);
  TEST_ALMOST_EQUAL_ABS(water, path.m_waterDistance, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(land, path.m_landDistance, 1e-6, ());
  TEST_GREATER_OR_EQUAL(water, 0.0, ());
  TEST_GREATER_OR_EQUAL(land, 0.0, ());
}

UNIT_TEST(Water_WindingRiver)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_lines = {{{P(0, 0), 1}, {P(0, 1), 2}, {P(1, 1), 3}, {P(1, 0), 4}}};
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(1, 0), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points, (std::vector<P>{P(0, 0), P(0, 1), P(1, 1), P(1, 0)}), ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_StraightRiverPartialSegment)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_lines = {{{P(0, 0), 1}, {P(2, 0), 2}}};
  Path path;
  TEST(FindPath(geometry, P(0.5, 0), P(1.5, 0), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points, (std::vector<P>{P(0.5, 0), P(1.5, 0)}), ());
  CheckAccounting(path);
}

UNIT_TEST(Water_JunctionChoosesShortestConnectedPath)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_lines = {{{P(0, 0), 1}, {P(1, 0), 2}, {P(2, 0), 3}},
                      {{P(0, 0), 1}, {P(1, 2), 4}, {P(2, 0), 3}},
                      {{P(1, 0), 2}, {P(1, -1), 5}}};
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(1, -1), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points, (std::vector<P>{P(0, 0), P(1, 0), P(1, -1)}), ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
}

UNIT_TEST(Water_LakeDirect)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 0, 0, 3, 3);
  Path path;
  TEST(FindPath(geometry, P(0.5, 1), P(2.5, 2), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_IslandCannotBecomeLandShortcut)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 0, 0, 3, 1);
  Rectangle(geometry, 0, 2, 3, 3);
  Rectangle(geometry, 0, 1, 1, 2);
  Rectangle(geometry, 2, 1, 3, 2);
  Path path;
  TEST(FindPath(geometry, P(0.5, 1.5), P(2.5, 1.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_GREATER(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  // Every open segment midpoint must stay outside the interior of the island.
  for (size_t i = 1; i < path.m_points.size(); ++i)
  {
    auto const p = (path.m_points[i - 1] + path.m_points[i]) * 0.5;
    TEST(!(p.x > 1 && p.x < 2 && p.y > 1 && p.y < 2), (p));
  }
  CheckAccounting(path);
}

UNIT_TEST(Water_RiverEntersLakeBetweenVertices)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 1, 0, 2, 2);
  geometry.m_lines = {{{P(0, 1), 1}, {P(3, 1), 2}}};
  Path path;
  TEST(FindPath(geometry, P(0, 1), P(1.5, 1.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_DisconnectedLakesOneExplicitGap)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 0, 0, 1, 1);
  Rectangle(geometry, 2, 0, 3, 1);
  Path path;
  TEST(FindPath(geometry, P(0.5, 0.5), P(2.5, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(std::count(path.m_landGaps.begin(), path.m_landGaps.end(), true), 1, ());
  TEST_GREATER(path.m_waterDistance, 0.0, ());
  TEST_GREATER(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_DisconnectedRiversTransferOnlyAtSourceEndpoints)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_lines = {{{P(0, 0), 1}, {P(0, 1), 2}, {P(0, 3), 3}}, {{P(1, 0), 4}, {P(1, 1), 5}, {P(1, 3), 6}}};
  Path path;
  TEST(FindPath(geometry, P(0, 1), P(1, 1), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(std::count(path.m_landGaps.begin(), path.m_landGaps.end(), true), 1, ());
  for (size_t i = 0; i < path.m_landGaps.size(); ++i)
  {
    if (!path.m_landGaps[i])
      continue;
    TEST_EQUAL(path.m_points[i], P(0, 0), ());
    TEST_EQUAL(path.m_points[i + 1], P(1, 0), ());
  }
  TEST_GREATER(path.m_waterDistance, 0.0, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_ExplicitLandPointRejectedButGpsProjects)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 0, 0, 1, 1);
  Path path;
  TEST(FindPath(geometry, P(-1, 0.5), P(0.5, 0.5), false, kNeverCancelled, path) == ResultCode::OutsideWater, ());
  TEST(path.m_points.empty(), ());
  TEST(FindPath(geometry, P(-1, 0.5), P(0.5, 0.5), true, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points.front(), P(0, 0.5), ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST(FindPath(geometry, P(0.5, 0.5), P(-1, 0.5), true, kNeverCancelled, path) == ResultCode::OutsideWater, ());
}

UNIT_TEST(Water_IncompleteGeometryNeverCreatesGap)
{
  Geometry geometry;
  Rectangle(geometry, 0, 0, 1, 1);
  Rectangle(geometry, 2, 0, 3, 1);
  Path path;
  TEST(FindPath(geometry, P(0.5, 0.5), P(2.5, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
}

UNIT_TEST(Water_Cancellation)
{
  Geometry geometry;
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(1, 1), false, [] { return true; }, path) == ResultCode::Cancelled, ());
  TEST(path.m_points.empty(), ());
}

UNIT_TEST(Water_SharedTriangleEdgeUsesConsistentArithmetic)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, -86.69698086408202, -13.777257972089522, -85.7789437760374, -12.97660607294521);
  Path path;
  TEST(FindPath(geometry, P(-86.60517715527756, -13.216801642688504), P(-85.87074748484186, -13.537062402346228), false,
                kNeverCancelled, path) == ResultCode::Success,
       ());
  TEST_EQUAL(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
}

UNIT_TEST(Water_ExcessiveGeometryIsUnavailable)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_triangles.resize(5000);
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(1, 1), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
}

UNIT_TEST(Water_DetailedRiversDoNotExhaustLakeVisibilityBudget)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 0, 0, 1, 1);
  auto & river = geometry.m_lines.emplace_back();
  for (uint64_t i = 0; i < 1500; ++i)
    river.push_back({P(2 + i * 0.0001, 2 + (i % 2) * 0.0001), i + 1});
  Path path;
  TEST(FindPath(geometry, P(0.2, 0.5), P(0.8, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points.size(), 2, ());
  TEST(FindPath(geometry, river.front().m_point, river.back().m_point, false, kNeverCancelled, path) ==
           ResultCode::Success,
       ());
  TEST_EQUAL(path.m_points.size(), river.size(), ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_UnknownGeometryCannotBecomeWaterOrLand)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 0, 0, 3, 1);
  geometry.m_unknownAreas.emplace_back(1, 0, 2, 1);
  Path path;
  TEST(FindPath(geometry, P(0.5, 0.5), P(2.5, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
  TEST(FindPath(geometry, P(1.5, 0.5), P(2.5, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  // A short certified path remains usable even with an unknown region in this lake.
  TEST(FindPath(geometry, P(0.1, 0.5), P(0.2, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
}

UNIT_TEST(Water_UnknownShortcutCannotCertifyDetour)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 0, 0, 3, 1);
  Rectangle(geometry, 0, 2, 3, 3);
  Rectangle(geometry, 0, 1, 1, 2);
  Rectangle(geometry, 2, 1, 3, 2);
  geometry.m_unknownAreas.emplace_back(1, 1, 2, 2);
  Path path;
  TEST(FindPath(geometry, P(0.5, 1.5), P(2.5, 1.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
}

UNIT_TEST(Water_UnknownGapAndUnknownFrontierAreRejected)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_lines = {{{P(0, 0), 1}, {P(0, 1), 2}}, {{P(3, 0), 3}, {P(3, 1), 4}}};
  geometry.m_unknownAreas.emplace_back(1, -1, 2, 2);
  Path path;
  TEST(FindPath(geometry, P(0, 0.5), P(3, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  geometry.m_unknownAreas = {m2::RectD(5, 5, 6, 6)};
  TEST(FindPath(geometry, P(0, 0.5), P(3, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_GREATER(path.m_landDistance, 0, ());
  geometry.m_unknownAreas = {m2::RectD(0, 0.8, 1, 1)};
  TEST(FindPath(geometry, P(0, 0.5), P(3, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
}

UNIT_TEST(Water_SurfaceVisibilityRemainsBounded)
{
  Geometry geometry;
  geometry.m_complete = true;
  for (size_t i = 0; i < 400; ++i)
    geometry.m_triangles.push_back({P(i, 0), P(i + 0.5, 0), P(i, 1)});
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(0, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
}

UNIT_TEST(Water_UnknownGapCannotJustifyLongerTransfer)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_lines = {{{P(0, 0), 1}, {P(0, 1), 2}}, {{P(3, 0), 3}, {P(3, 1), 4}}};
  geometry.m_unknownAreas.emplace_back(1, -0.1, 2, 0.1);
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(3, 0), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
}

UNIT_TEST(Water_GpsCannotSkipCloserUnknownWater)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, 2, 0, 3, 1);
  geometry.m_unknownAreas.emplace_back(-0.1, 0, 0.1, 1);
  Path path;
  TEST(FindPath(geometry, P(0, 0.5), P(2.1, 0.5), true, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
}

UNIT_TEST(Water_RiverLakePortalAtRealMercatorScale)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, -2.9787849, 65.171, -2.978, 65.173);
  P const start(-2.9788832, 65.172057);
  P const finish(-2.9787, 65.1723);
  geometry.m_lines = {{{start, 1}, {P(-2.9787008, 65.1720142), 2}}};
  Path path;
  TEST(FindPath(geometry, start, finish, false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST_LESS(path.m_waterDistance, 50.0, ());
  TEST(FindPath(geometry, finish, start, false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST_LESS(path.m_waterDistance, 50.0, ());
}

UNIT_TEST(Water_RiverLakePortalRounding)
{
  Geometry geometry;
  geometry.m_complete = true;
  double const left = 0.7659226110346822;
  Rectangle(geometry, left, 0, 1.4601580216993968, 1);
  P const start(-0.22492414421756202, 0.5);
  geometry.m_lines = {{{start, 1}, {{2.113273353142489, 0.5}, 2}}};
  Path path;
  TEST(FindPath(geometry, start, P(left + 0.01, 0.6), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  // Missing the entrance forces a >250 km detour through the far shore.
  TEST_LESS(path.m_waterDistance, 150000.0, ());
  CheckAccounting(path);
}
}  // namespace water_geometry_test
