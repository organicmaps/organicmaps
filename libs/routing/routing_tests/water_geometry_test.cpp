#include "testing/testing.hpp"

#include "routing/water_geometry.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <limits>

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

UNIT_TEST(Water_StandardTransfersOnlyAtStoredLineEndpoints)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  geometry.m_lines = {{{P(0, 0)}, {P(0, 1)}, {P(0, 3)}}, {{P(1, 0)}, {P(1, 1)}, {P(1, 3)}}};
  Path path;
  TEST(FindPath(geometry, P(0, 1), P(1, 1), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(std::count(path.m_landGaps.begin(), path.m_landGaps.end(), true), 1, ());
  for (size_t i = 0; i < path.m_landGaps.size(); ++i)
    if (path.m_landGaps[i])
    {
      TEST_EQUAL(path.m_points[i], P(0, 0), ());
      TEST_EQUAL(path.m_points[i + 1], P(1, 0), ());
    }
  CheckAccounting(path);
  // Artificial extraction endpoints cannot turn the selected interior points into transfers.
  for (auto & line : geometry.m_lines)
  {
    line.front().m_mayTransfer = false;
    line.back().m_mayTransfer = false;
  }
  TEST(FindPath(geometry, P(0, 1), P(1, 1), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
}

UNIT_TEST(Water_StandardTransfersNeverMoveToShoreline)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  Rectangle(geometry, 0, 0, 1, 1);
  Rectangle(geometry, 2, 0, 3, 1);
  Path path;
  TEST(FindPath(geometry, P(0.5, 0.5), P(2.5, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  // Even with endpoints inside the lakes, the dry interval starts at a shoreline cut.
  geometry.m_lines = {{{P(0.25, 0.5)}, {P(0.75, 0.5)}}, {{P(2.25, 0.5)}, {P(2.75, 0.5)}}};
  TEST(FindPath(geometry, P(0.5, 0.5), P(2.5, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
}

UNIT_TEST(Water_StandardTransferKeepsExactEndpointCoordinates)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  auto const a = P(-1, 0);
  auto const b = P(-1e-20, 0);
  geometry.m_lines = {{{a}, {P(-1, 1)}}, {{b}, {P(-1e-20, 1)}}};
  Path path;
  TEST(FindPath(geometry, a, b, false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_points.front(), a, ());
  TEST_EQUAL(path.m_points.back(), b, ());
  TEST(path.m_landGaps.front(), ());
  CheckAccounting(path);
}

UNIT_TEST(Water_KnownUndergroundBreakAllowsLocalPortageWithOpenFrontier)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  geometry.m_bounds = m2::RectD(-0.002, -0.002, 0.005, 0.002);
  geometry.m_lines = {{{P(-0.002, 0), 0, false}, {P(0.001, 0)}}, {{P(0.002, 0)}, {P(0.005, 0), 0, false}}};
  geometry.m_portageAreas.emplace_back(0.0009, -0.0001, 0.0021, 0.0001);
  Path path;
  auto const find = [&] { return FindPath(geometry, P(0, 0), P(0.003, 0), false, kNeverCancelled, path); };
  TEST(find() == ResultCode::Success, ());
  TEST(!path.m_shortestConfirmed, ());
  TEST_GREATER(path.m_waterDistance, 0.0, ());
  TEST_GREATER(path.m_landDistance, 0.0, ());
  for (size_t i = 0; i < path.m_landGaps.size(); ++i)
    if (path.m_landGaps[i])
    {
      TEST_EQUAL(path.m_points[i], P(0.001, 0), ());
      TEST_EQUAL(path.m_points[i + 1], P(0.002, 0), ());
    }
  CheckAccounting(path);
  geometry.m_unknownAreas.emplace_back(0.0014, -0.0001, 0.0016, 0.0001);
  TEST(find() == ResultCode::Unavailable, ());
  geometry.m_unknownAreas.clear();
  geometry.m_lines[0].back().m_mayTransfer = false;
  TEST(find() == ResultCode::Unavailable, ());
  geometry.m_lines[0].back().m_mayTransfer = true;
  geometry.m_portageAreas = {{0.0035, -0.0001, 0.0036, 0.0001}};
  TEST(find() == ResultCode::Unavailable, ());
  geometry.m_portageAreas.clear();
  TEST(find() == ResultCode::Unavailable, ());
}

UNIT_TEST(Water_KnownPortageKeepsEndpointsInsideWaterSurfaces)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  geometry.m_bounds = m2::RectD(-0.002, -0.002, 0.005, 0.002);
  geometry.m_lines = {{{P(-0.002, 0), 0, false}, {P(0.001, 0)}}, {{P(0.002, 0)}, {P(0.005, 0), 0, false}}};
  Rectangle(geometry, -0.001, -0.0005, 0.0012, 0.0005);
  Rectangle(geometry, 0.0018, -0.0005, 0.004, 0.0005);
  geometry.m_portageAreas.emplace_back(0.001, -0.0001, 0.002, 0.0001);
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(0.003, 0), false, kNeverCancelled, path) == ResultCode::Success, ());
  for (size_t i = 0; i < path.m_landGaps.size(); ++i)
    if (path.m_landGaps[i])
    {
      TEST_EQUAL(path.m_points[i], P(0.001, 0), ());
      TEST_EQUAL(path.m_points[i + 1], P(0.002, 0), ());
    }
  TEST_GREATER(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}
UNIT_TEST(Water_KnownPortageStillPrefersConnectedWater)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  geometry.m_bounds = m2::RectD(-0.002, -0.002, 0.005, 0.002);
  geometry.m_lines = {{{P(-0.002, 0), 0, false}, {P(0.001, 0)}}, {{P(0.002, 0)}, {P(0.005, 0), 0, false}}};
  geometry.m_portageAreas.emplace_back(0.0009, -0.0001, 0.0021, 0.0001);
  Rectangle(geometry, 0.0005, -0.0005, 0.0025, 0.0005);
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(0.003, 0), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
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
  geometry.m_triangles.resize(33000);
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
  TEST(FindPath(geometry, P(0.5, 1.5), P(2.5, 1.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST(!path.m_points.empty(), ());
  TEST(!path.m_shortestConfirmed, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
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

UNIT_TEST(Water_LocalPathDoesNotBuildUnrelatedVisibilityEdges)
{
  Geometry geometry;
  geometry.m_complete = true;
  for (size_t i = 0; i < 2048; ++i)
  {
    P const a(i * 0.001, 0), b((i + 1) * 0.001, 0);
    P const c((i + 1) * 0.001, (i + 1) % 2 ? 2 : 1), d(i * 0.001, i % 2 ? 2 : 1);
    geometry.m_triangles.push_back({a, b, c});
    geometry.m_triangles.push_back({a, c, d});
  }
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(0, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_LongPathInNarrowCoverage)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_bounds = m2::RectD(0, -1, 10, 1);
  Rectangle(geometry, 0, -1, 10, 1);
  Path path;
  TEST(FindPath(geometry, P(0.5, 0), P(9.5, 0), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
}

UNIT_TEST(Water_LongIslandDetourInNarrowCoverage)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_bounds = m2::RectD(0, -1, 10, 1);
  for (auto const x : {0.0, 6.0})
  {
    Rectangle(geometry, x, -1, x + 4, -0.5);
    Rectangle(geometry, x, -0.5, x + 4, 0.5);
    Rectangle(geometry, x, 0.5, x + 4, 1);
  }
  Rectangle(geometry, 4, -1, 6, -0.5);
  Rectangle(geometry, 4, 0.5, 6, 1);
  Path path;
  TEST(FindPath(geometry, P(0.5, 0), P(9.5, 0), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_GREATER(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_IndexedVisibilityHandlesCancellationNearZero)
{
  Geometry geometry;
  geometry.m_complete = true;
  Rectangle(geometry, -2, 0, 1e-16, 1);
  Rectangle(geometry, 1e-16, 0, 2, 1);
  geometry.m_triangleIndex = std::make_shared<m4::Tree<size_t>>();
  for (size_t i = 0; i < geometry.m_triangles.size(); ++i)
  {
    m2::RectD bounds;
    for (auto const & point : geometry.m_triangles[i])
      bounds.Add(point);
    geometry.m_triangleIndex->Add(i, bounds);
  }
  Path path;
  TEST(FindPath(geometry, P(-1, 0.5), P(1, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points.size(), 2, ());
  CheckAccounting(path);
}

UNIT_TEST(Water_IndexedVisibilityDoesNotBridgeThinLand)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  Rectangle(geometry, -2, 0, -1e-15, 1);
  Rectangle(geometry, 1e-15, 0, 2, 1);
  geometry.m_triangleIndex = std::make_shared<m4::Tree<size_t>>();
  for (size_t i = 0; i < geometry.m_triangles.size(); ++i)
  {
    m2::RectD bounds;
    for (auto const & point : geometry.m_triangles[i])
      bounds.Add(point);
    geometry.m_triangleIndex->Add(i, bounds);
  }
  Path path;
  TEST(FindPath(geometry, P(-1, 0.5), P(1, 0.5), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
}

UNIT_TEST(Water_InteriorMeshDoesNotExhaustVisibilityBudget)
{
  Geometry geometry;
  geometry.m_complete = true;
  for (size_t x = 0; x < 33; ++x)
    for (size_t y = 0; y < 33; ++y)
      Rectangle(geometry, x, y, x + 1, y + 1);
  geometry.m_triangleIndex = std::make_shared<m4::Tree<size_t>>();
  for (size_t i = 0; i < geometry.m_triangles.size(); ++i)
  {
    m2::RectD bounds;
    for (auto const & point : geometry.m_triangles[i])
      bounds.Add(point);
    geometry.m_triangleIndex->Add(i, bounds);
  }
  Path path;
  TEST(FindPath(geometry, P(0.5, 0.5), P(32.5, 32.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
}

UNIT_TEST(Water_SubmergedRiverBendsDoNotExhaustVisibilityBudget)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_hasSourceTopology = false;
  for (size_t x = 0; x < 3; ++x)
    for (size_t y = 0; y < 3; ++y)
      if (x != 1 || y != 1)
        Rectangle(geometry, x, y, x + 1, y + 1);
  // A detailed line inside a lake adds no obstacles or surface transitions.
  auto & line = geometry.m_lines.emplace_back();
  for (size_t row = 0; row < 60; ++row)
    for (size_t column = 0; column < 80; ++column)
    {
      auto const x = row % 2 == 0 ? column : 79 - column;
      line.push_back({P(0.1 + 0.8 * x / 79, 1.1 + 0.8 * row / 59)});
    }
  geometry.m_triangleIndex = std::make_shared<m4::Tree<size_t>>();
  for (size_t i = 0; i < geometry.m_triangles.size(); ++i)
  {
    m2::RectD bounds;
    for (auto const & point : geometry.m_triangles[i])
      bounds.Add(point);
    geometry.m_triangleIndex->Add(i, bounds);
  }
  geometry.m_triangleIndex->Optimize();
  Path path;
  auto const start = P(0.5, 1.5), finish = P(2.5, 1.5);
  TEST(FindPath(geometry, start, finish, false, kNeverCancelled, path) == ResultCode::Success, ());
  double best = std::numeric_limits<double>::max();
  for (double y : {1.0, 2.0})
    best = std::min(best, mercator::DistanceOnEarth(start, P(1, y)) + mercator::DistanceOnEarth(P(1, y), P(2, y)) +
                              mercator::DistanceOnEarth(P(2, y), finish));
  TEST_ALMOST_EQUAL_ABS(path.m_waterDistance, best, 1e-5, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}
UNIT_TEST(Water_UnrelatedFrontierCannotInvalidateConnectedRoute)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_bounds = m2::RectD(0, 0, 10, 10);
  geometry.m_lines = {{{P(2, 2), 1}, {P(2, 6), 2}, {P(8, 6), 3}, {P(8, 2), 4}},
                      {{P(2, 6), 2}, {P(2, 10), 5}},
                      {{P(4, 0), 6}, {P(4, 0.1), 7}}};
  // Only the distant top frontier is reachable from these checkpoints.
  // The separate bottom waterway cannot supply a shorter outside excursion.
  Path path;
  TEST(FindPath(geometry, P(2, 2), P(8, 2), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points, (std::vector<P>{P(2, 2), P(2, 6), P(8, 6), P(8, 2)}), ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
}
UNIT_TEST(Water_FrontierCostIncludesWaterDetours)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_bounds = m2::RectD(0, 0, 10, 10);
  geometry.m_lines = {{{P(2, 2), 1}, {P(2, 6), 2}, {P(8, 6), 3}, {P(8, 2), 4}}, {{P(2, 6), 2}, {P(0, 6), 5}}};
  // The straight-line bound via the left frontier is too optimistic. Reaching
  // that frontier along water costs more than the selected connected route.
  Path path;
  TEST(FindPath(geometry, P(2, 2), P(8, 2), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points, (std::vector<P>{P(2, 2), P(2, 6), P(8, 6), P(8, 2)}), ());
  CheckAccounting(path);
}

UNIT_TEST(Water_CheaperFrontierStillRequiresMoreCoverage)
{
  Geometry geometry;
  geometry.m_complete = true;
  geometry.m_bounds = m2::RectD(0, 0, 10, 10);
  geometry.m_lines = {{{P(2, 2), 1}, {P(2, 6), 2}, {P(8, 6), 3}, {P(8, 2), 4}}, {{P(2, 2), 1}, {P(0, 2), 5}}};
  Path path;
  TEST(FindPath(geometry, P(2, 2), P(8, 2), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST(!path.m_points.empty(), ());
  TEST(!path.m_shortestConfirmed, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  CheckAccounting(path);
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

UNIT_TEST(Water_RiverLakePortalAlongNearlyParallelShore)
{
  P const a(5.2517934534832306781, 61.71968312352660746);
  P const b(5.2537782881690304748, 61.720865977710985817);
  P const entry(5.2535980167382571844, 61.7207585456826463202);
  for (bool const indexed : {false, true})
    for (bool const reversed : {false, true})
    {
      Geometry geometry;
      geometry.m_complete = true;
      geometry.m_lines = {{{a, 1}, {b, 2}}};
      if (reversed)
        std::reverse(geometry.m_lines.front().begin(), geometry.m_lines.front().end());
      // This standard-MWM shore amplifies rounding of the intersection parameter.
      geometry.m_triangles = {{P(5.2537782881690304748, 61.720815015739304954),
                               P(5.2537514660786825971, 61.720852566665797667),
                               P(5.2528234217526232896, 61.720283938350405606)}};
      if (indexed)
      {
        geometry.m_triangleIndex = std::make_shared<m4::Tree<size_t>>();
        m2::RectD bounds;
        for (auto const & point : geometry.m_triangles.front())
          bounds.Add(point);
        geometry.m_triangleIndex->Add(0, bounds);
      }
      // The shore destination also detects excessive inward movement of the portal.
      for (P const finish : {a + (b - a) * 0.95, geometry.m_triangles.front()[2]})
      {
        auto const distance = mercator::DistanceOnEarth(a, entry) + mercator::DistanceOnEarth(entry, finish);
        for (bool const reverseRoute : {false, true})
        {
          Path path;
          TEST(FindPath(geometry, reverseRoute ? finish : a, reverseRoute ? a : finish, false, kNeverCancelled, path) ==
                   ResultCode::Success,
               (indexed, reversed, reverseRoute));
          TEST_EQUAL(path.m_landDistance, 0.0, ());
          TEST_ALMOST_EQUAL_ABS(path.m_waterDistance, distance, 0.001, ());
          CheckAccounting(path);
        }
      }
    }
}
}  // namespace water_geometry_test

UNIT_TEST(Water_ShoreApproachRemainsConnectedToSurface)
{
  for (size_t i = 1; i <= 64; ++i)
  {
    routing::water::Geometry geometry;
    geometry.m_complete = true;
    m2::PointD const a(5.18, 61.66), b(5.18 + 0.001 * i, 61.66 + 0.0031), c(5.185, 61.67);
    geometry.m_triangles = {{a, b, c}};
    auto const center = (a + b + c) / 3.0;
    auto const shore = a + (b - a) * 0.371;
    auto const direction = b - a;
    m2::PointD normal(direction.y, -direction.x);
    if (normal.x * (center - shore).x + normal.y * (center - shore).y > 0)
      normal = -normal;
    auto const outside = shore + normal * 0.01;
    auto const entry = routing::water::FindApproachPoint(geometry, outside, [] { return false; });
    TEST(entry.has_value(), (i));
    routing::water::Path path;
    TEST(routing::water::FindPath(geometry, *entry, center, false, [] { return false; }, path) ==
             routing::water::ResultCode::Success,
         (i, *entry));
    TEST_EQUAL(path.m_landDistance, 0.0, (i));
    TEST_LESS(mercator::DistanceOnEarth(shore, *entry), 0.001, (i));
  }
}

UNIT_TEST(Water_SurfaceTJunctionKeepsWaterDetourConnected)
{
  using namespace water_geometry_test;
  for (bool indexed : {false, true})
  {
    for (double gap : {0.0, 0.00001})
    {
      Geometry geometry;
      geometry.m_complete = true;
      geometry.m_hasSourceTopology = false;
      Rectangle(geometry, 0, 0, 1, 1);
      Rectangle(geometry, 1 + gap, 0.25, 2, 0.75);
      Rectangle(geometry, 1.75, 0.75, 2, 2);
      if (indexed)
      {
        geometry.m_triangleIndex = std::make_shared<m4::Tree<size_t>>();
        for (size_t i = 0; i < geometry.m_triangles.size(); ++i)
        {
          m2::RectD bounds;
          for (auto const & p : geometry.m_triangles[i])
            bounds.Add(p);
          geometry.m_triangleIndex->Add(i, bounds);
        }
        geometry.m_triangleIndex->Optimize();
      }
      Path path;
      auto const code = FindPath(geometry, P(0.5, 0.5), P(1.9, 1.5), false, kNeverCancelled, path);
      if (gap != 0)
      {
        TEST(code == ResultCode::Unavailable, (indexed));
        continue;
      }
      TEST(code == ResultCode::Success, (indexed));
      TEST_EQUAL(path.m_landDistance, 0.0, ());
      CheckAccounting(path);
    }
  }
}
