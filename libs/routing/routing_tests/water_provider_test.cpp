#include "testing/testing.hpp"

#include "routing/water_geometry_provider.hpp"

#include "coding/writer.hpp"

namespace water_provider_test
{
using P = m2::PointD;
using routing::water::Path;
using routing::water::ResultCode;
auto const kNeverCancelled = [] { return false; };

::water::Record Area(std::vector<std::vector<P>> rings)
{
  return {::water::Kind::Area, std::move(rings), {}};
}

std::vector<uint8_t> Section(std::vector<::water::Record> const & records)
{
  std::vector<uint8_t> bytes;
  MemWriter writer(bytes);
  ::water::WriteSection(writer, {{P(-10, -10), P(10, -10), P(10, 10), P(-10, 10)}}, records);
  return bytes;
}

routing::water::Geometry Extract(std::vector<uint8_t> const & bytes, m2::RectD bounds = {-5, -5, 5, 5})
{
  ::water::Reader reader(ReaderPtr<::Reader>(std::make_unique<MemReader>(bytes.data(), bytes.size())));
  return routing::WaterGeometryProvider::Extract(reader, bounds, kNeverCancelled);
}

UNIT_TEST(WaterProvider_SectionUnionPreservesIsland)
{
  auto const bytes = Section({Area({{P(0, 0), P(3, 0), P(3, 3), P(0, 3)}, {P(1, 1), P(2, 1), P(2, 2), P(1, 2)}}),
                              Area({{P(0, 0), P(0.75, 0), P(0.75, 3), P(0, 3)}})});
  auto const geometry = Extract(bytes);
  TEST(geometry.m_complete, ());
  Path path;
  TEST(FindPath(geometry, P(0.5, 1.5), P(2.5, 1.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST_GREATER(path.m_points.size(), 2, ());
  TEST(FindPath(geometry, P(1.5, 1.5), P(2.5, 1.5), false, kNeverCancelled, path) == ResultCode::OutsideWater, ());
}

UNIT_TEST(WaterProvider_OpenFrontierNeverBecomesGap)
{
  auto const bytes = Section(
      {{::water::Kind::Line, {{P(0, -8), P(0, 8)}}, {1, 2}}, {::water::Kind::Line, {{P(1, -8), P(1, 8)}}, {3, 4}}});
  auto geometry = Extract(bytes);
  geometry.m_hasSourceTopology = false;
  TEST(geometry.m_complete, ());
  for (auto const & line : geometry.m_lines)
  {
    TEST(!line.front().m_mayTransfer, ());
    TEST(!line.back().m_mayTransfer, ());
  }
  Path path;
  TEST(FindPath(geometry, P(0, 0), P(1, 0), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
  TEST(path.m_points.empty(), ());
  TEST(FindPath(geometry, P(0, 0), P(0, 1), false, kNeverCancelled, path) == ResultCode::Success, ());
}

UNIT_TEST(WaterProvider_SourceEndpointsSurviveExtraction)
{
  auto const bytes = Section({{::water::Kind::Line, {{P(0, 0), P(0, 1), P(0, 3)}}, {1, 2, 3}},
                              {::water::Kind::Line, {{P(1, 0), P(1, 1), P(1, 3)}}, {4, 5, 6}}});
  auto const geometry = Extract(bytes);
  Path path;
  TEST(FindPath(geometry, P(0, 1), P(1, 1), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_GREATER(path.m_waterDistance, 0, ());
  for (size_t i = 0; i < path.m_landGaps.size(); ++i)
    if (path.m_landGaps[i])
    {
      TEST_EQUAL(path.m_points[i], P(0, 0), ());
      TEST_EQUAL(path.m_points[i + 1], P(1, 0), ());
    }
}

UNIT_TEST(WaterProvider_OutsideCoverageUnsupportedAndCancellation)
{
  auto const bytes = Section({Area({{P(0, 0), P(3, 0), P(3, 3), P(0, 3)}})});
  TEST(!Extract(bytes, {-11, -1, 1, 1}).m_complete, ());
  ::water::Reader reader(ReaderPtr<::Reader>(std::make_unique<MemReader>(bytes.data(), bytes.size())));
  TEST(!routing::WaterGeometryProvider::Extract(reader, {-5, -5, 5, 5}, [] { return true; }).m_complete, ());
  auto const unsupported = Section({{::water::Kind::Unsupported, {{P(0, 0), P(1, 1)}}, {}}});
  auto const unknown = Extract(unsupported);
  TEST(unknown.m_complete, ());
  TEST_EQUAL(unknown.m_unknownAreas.size(), 1, ());
  Path path;
  TEST(FindPath(unknown, P(0, 0), P(1, 1), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
}

UNIT_TEST(WaterProvider_UnrelatedCulvertDoesNotDisableLake)
{
  auto const bytes =
      Section({Area({{P(0, 0), P(1, 0), P(1, 1), P(0, 1)}}), {::water::Kind::Unsupported, {{P(3, 3), P(4, 4)}}, {}}});
  auto const geometry = Extract(bytes);
  TEST(geometry.m_complete, ());
  TEST_EQUAL(geometry.m_unknownAreas.size(), 1, ());
  Path path;
  TEST(FindPath(geometry, P(0.2, 0.5), P(0.8, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
}

UNIT_TEST(WaterProvider_IndexedShorelineAndRiverEndpointContact)
{
  auto const bytes =
      Section({Area({{P(1, 0), P(2, 0), P(2, 2), P(1, 2)}}), {::water::Kind::Line, {{P(0, 1), P(1, 1)}}, {1, 2}}});
  auto const geometry = Extract(bytes);
  Path path;
  TEST(FindPath(geometry, P(0, 1), P(1.5, 1.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST(FindPath(geometry, P(1, 0), P(2, 0), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
}

UNIT_TEST(WaterProvider_InvalidAreaIsUnknownLocally)
{
  auto const bytes =
      Section({Area({{P(0, 0), P(1, 0), P(1, 1), P(0, 1)}}), Area({{P(2, 2), P(4, 4), P(4, 2), P(2, 4)}})});
  auto const geometry = Extract(bytes);
  TEST(geometry.m_complete, ());
  TEST_EQUAL(geometry.m_unknownAreas.size(), 1, ());
  Path path;
  TEST(FindPath(geometry, P(0.2, 0.5), P(0.8, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST(FindPath(geometry, P(0.5, 0.5), P(3, 3), false, kNeverCancelled, path) != ResultCode::Success, ());
}

UNIT_TEST(WaterProvider_SpatialIndexSkipsUnrelatedLargeFeatures)
{
  auto far = Area({std::vector<P>(20000, P(8, 8))});
  auto const bytes = Section({Area({{P(0, 0), P(3, 0), P(3, 3), P(0, 3)}}), far});
  TEST(Extract(bytes).m_complete, ());
}
UNIT_TEST(WaterProvider_CollinearSurfaceDoesNotAbortOrBecomeWater)
{
  auto const bytes = Section({Area({{P(0, 0), P(1, 0), P(2, 0)}}), Area({{P(3, 0), P(4, 0), P(4, 1), P(3, 1)}})});
  auto const geometry = Extract(bytes);
  TEST(geometry.m_complete, ());
  TEST(!geometry.m_unknownAreas.empty(), ());
  Path path;
  TEST(FindPath(geometry, P(3.25, 0.5), P(3.75, 0.5), false, kNeverCancelled, path) == ResultCode::Success, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST(FindPath(geometry, P(0.5, 0), P(1.5, 0), false, kNeverCancelled, path) == ResultCode::Unavailable, ());
}
UNIT_TEST(WaterProvider_CoarseBorderCellsDoNotCutCoveredWater)
{
  for (bool missing : {false, true})
  {
    routing::water::Geometry geometry;
    geometry.m_complete = true;
    geometry.m_bounds = m2::RectD(0, 0, 1, 1);
    P const start(0.2, 0.21), finish(0.8, 0.81);
    geometry.m_lines = {{{start, 1}, {finish, 2}}};
    size_t checks = 0;
    routing::WaterGeometryProvider::SetCoverage(geometry, {start, finish}, [&](auto const & rect)
    {
      ++checks;
      return rect.minY() > rect.maxX() && (!missing || rect.maxX() < 0.49 || rect.minX() > 0.51);
    }, kNeverCancelled);
    TEST(!geometry.m_coverageGaps.empty(), ());
    Path path;
    auto const code = FindPath(geometry, start, finish, false, kNeverCancelled, path);
    TEST(code == (missing ? ResultCode::Unavailable : ResultCode::Success), (missing));
    if (!missing)
      TEST_EQUAL(path.m_landDistance, 0.0, ());
    auto const before = checks;
    TEST(FindPath(geometry, start, finish, false, kNeverCancelled, path) == code, ());
    TEST_EQUAL(checks, before, ());
  }
}
UNIT_TEST(WaterProvider_SurfaceCorridorDetoursWithoutCrossingLand)
{
  // A wall ending well north of both pins requires leaving their initial rectangle.
  auto const bytes =
      Section({Area({{P(-2, -2), P(-0.2, -2), P(-0.2, 2), P(-2, 2)}}),
               Area({{P(0.2, -2), P(2, -2), P(2, 2), P(0.2, 2)}}), Area({{P(-2, 1), P(2, 1), P(2, 2), P(-2, 2)}})});
  size_t loads = 0;
  auto const regions =
      routing::WaterGeometryProvider::FindWaterCorridor({P(-0.5, 0), P(0.5, 0)}, [&](auto const & bounds)
  {
    ++loads;
    return Extract(bytes, bounds);
  }, kNeverCancelled);
  TEST(!regions.empty(), ());
  TEST_LESS_OR_EQUAL(loads, 1024, ());
  TEST(std::any_of(regions.begin(), regions.end(), [](auto const & r) { return r.maxY() > 1; }), ());
  TEST(std::any_of(regions.begin(), regions.end(), [](auto const & r) { return r.IsPointInside(P(-0.5, 0)); }), ());
  TEST(std::any_of(regions.begin(), regions.end(), [](auto const & r) { return r.IsPointInside(P(0.5, 0)); }), ());
}

UNIT_TEST(WaterProvider_SurfaceCorridorHonorsUnknownCoverageAndCancellation)
{
  auto const bytes = Section({Area({{P(-2, -2), P(2, -2), P(2, 2), P(-2, 2)}})});
  size_t loads = 0;
  auto const loader = [&](auto const & bounds)
  {
    ++loads;
    auto geometry = Extract(bytes, bounds);
    geometry.m_isCovered = [](auto const &, auto const &) { return false; };
    return geometry;
  };
  TEST(routing::WaterGeometryProvider::FindWaterCorridor({P(-0.5, 0), P(0.5, 0)}, loader, kNeverCancelled).empty(), ());
  TEST_LESS_OR_EQUAL(loads, 9, ());
  loads = 0;
  TEST(routing::WaterGeometryProvider::FindWaterCorridor({P(-0.5, 0), P(0.5, 0)}, loader, [] { return true; }).empty(),
       ());
  TEST_EQUAL(loads, 0, ());
}

UNIT_TEST(WaterProvider_SurfaceCorridorStopsAtWorkBudget)
{
  size_t loads = 0;
  auto const loader = [&](m2::RectD const & bounds)
  {
    ++loads;
    routing::water::Geometry geometry;
    geometry.m_complete = true;
    // Two unbounded seas separated by land: never claim they are connected.
    for (auto const & sea : {m2::RectD(-180, -180, -0.1, 180), m2::RectD(0.1, -180, 180, 180)})
    {
      auto r = bounds;
      if (r.Intersect(sea))
      {
        geometry.m_triangles.push_back({r.LeftBottom(), r.RightBottom(), r.RightTop()});
        geometry.m_triangles.push_back({r.LeftBottom(), r.RightTop(), r.LeftTop()});
      }
    }
    return geometry;
  };
  TEST(routing::WaterGeometryProvider::FindWaterCorridor({P(-0.5, 0), P(0.5, 0)}, loader, kNeverCancelled).empty(), ());
  TEST_LESS_OR_EQUAL(loads, 1024, ());
  TEST_GREATER(loads, 0, ());
  loads = 0;
  TEST(routing::WaterGeometryProvider::FindWaterCorridor({P(-0.5, 0), P(0.5, 0)}, loader, [&] { return loads >= 2; })
           .empty(),
       ());
  TEST_LESS_OR_EQUAL(loads, 2, ());
}

UNIT_TEST(WaterProvider_SelectedRegionsKeepUnloadedWaterUnknown)
{
  auto const bytes = Section({Area({{P(0, 0), P(0.003, 0), P(0.003, 0.002), P(0, 0.002)}})});
  for (bool gap : {false, true})
  {
    auto geometry = Extract(bytes, {0, 0, 0.003, 0.002});
    geometry.m_selective = true;
    double const right = gap ? 0.002 : 0.001;
    routing::WaterGeometryProvider::SetCoverage(geometry, {P(0.0005, 0.001), P(0.0025, 0.001)}, [](auto const &)
    { return true; }, kNeverCancelled, {{0, 0, 0.001, 0.002}, {right, 0, 0.003, 0.002}});
    TEST_EQUAL(geometry.m_isCovered(P(0.0005, 0.001), P(0.0025, 0.001)), !gap, ());
    TEST(geometry.m_isCovered(P(0.001, 0.0005), P(0.001, 0.0015)), ());
    Path path;
    auto const code = FindPath(geometry, P(0.0005, 0.001), P(0.0025, 0.001), false, kNeverCancelled, path, {}, true);
    TEST(code == (gap ? ResultCode::Unavailable : ResultCode::Success), ());
    if (!gap)
      TEST_EQUAL(path.m_landDistance, 0.0, ());
  }
}

}  // namespace water_provider_test

UNIT_TEST(WaterProvider_CorridorFollowsBentRiverWithoutCellCenters)
{
  using namespace water_provider_test;
  auto const bytes =
      Section({{::water::Kind::Line, {{P(-0.5, 0.013), P(-0.3, -0.7), P(0.3, -0.7), P(0.5, 0.017)}}, {1, 0, 0, 2}}});
  auto const regions = routing::WaterGeometryProvider::FindWaterCorridor(
      {P(-0.5, 0.013), P(0.5, 0.017)}, [&](auto const & bounds) { return Extract(bytes, bounds); }, kNeverCancelled);
  TEST(!regions.empty(), ());
  TEST(std::any_of(regions.begin(), regions.end(), [](auto const & r) { return r.minY() < -0.6; }), ());
}

UNIT_TEST(WaterProvider_CorridorKeepsDisconnectedComponentsSeparate)
{
  using namespace water_provider_test;
  auto const bytes = Section({{::water::Kind::Line, {{P(-0.5, 0.013), P(0.5, 0.013)}}, {1, 2}},
                              {::water::Kind::Line, {{P(-0.5, 0.017), P(0.5, 0.017)}}, {3, 4}}});
  auto const regions = routing::WaterGeometryProvider::FindWaterCorridor(
      {P(-0.5, 0.013), P(0.5, 0.017)}, [&](auto const & bounds) { return Extract(bytes, bounds); }, kNeverCancelled);
  TEST(regions.empty(), ());
}

UNIT_TEST(WaterProvider_CorridorSubdividesComplexTiles)
{
  using namespace water_provider_test;
  auto const bytes = Section({Area({{P(-1, -0.1), P(1, -0.1), P(1, 0.1), P(-1, 0.1)}})});
  size_t split = 0;
  auto const regions =
      routing::WaterGeometryProvider::FindWaterCorridor({P(-0.5, 0.013), P(0.5, 0.017)}, [&](auto const & bounds)
  {
    if (bounds.SizeX() > 0.1)
    {
      ++split;
      routing::water::Geometry geometry;
      geometry.m_resourceLimited = true;
      return geometry;
    }
    return Extract(bytes, bounds);
  }, kNeverCancelled);
  TEST(!regions.empty(), ());
  TEST_GREATER(split, 0, ());
  for (auto const & region : regions)
    TEST_LESS_OR_EQUAL(region.SizeX(), 0.1, ());
}

UNIT_TEST(WaterProvider_CorridorIsIndependentOfDirection)
{
  using namespace water_provider_test;
  auto const bytes =
      Section({Area({{P(-2, -2), P(-0.2, -2), P(-0.2, 2), P(-2, 2)}}),
               Area({{P(0.2, -2), P(2, -2), P(2, 2), P(0.2, 2)}}), Area({{P(-2, 1), P(2, 1), P(2, 2), P(-2, 2)}})});
  auto const loader = [&](auto const & bounds) { return Extract(bytes, bounds); };
  auto const forward =
      routing::WaterGeometryProvider::FindWaterCorridor({P(-0.5, 0), P(0.5, 0)}, loader, kNeverCancelled);
  auto const backward =
      routing::WaterGeometryProvider::FindWaterCorridor({P(0.5, 0), P(-0.5, 0)}, loader, kNeverCancelled);
  TEST(!forward.empty(), ());
  TEST_EQUAL(forward, backward, ());
}

UNIT_TEST(WaterProvider_CorridorStartOnTileBoundary)
{
  using namespace water_provider_test;
  // The source lies on the upper bank; water exists only in the tile below it.
  auto const bytes = Section({Area({{P(-0.1, -0.1), P(1, -0.1), P(1, 0), P(-0.1, 0)}})});
  auto const regions = routing::WaterGeometryProvider::FindWaterCorridor(
      {P(0, 0), P(0.5, -0.01)}, [&](auto const & bounds) { return Extract(bytes, bounds); }, kNeverCancelled);
  TEST(!regions.empty(), ());
}
