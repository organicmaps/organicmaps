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
  auto const geometry = Extract(bytes);
  TEST(geometry.m_complete, ());
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
  TEST(!Extract(unsupported).m_complete, ());
}

UNIT_TEST(WaterProvider_SpatialIndexSkipsUnrelatedLargeFeatures)
{
  auto far = Area({std::vector<P>(20000, P(8, 8))});
  auto const bytes = Section({Area({{P(0, 0), P(3, 0), P(3, 3), P(0, 3)}}), far});
  TEST(Extract(bytes).m_complete, ());
}
}  // namespace water_provider_test
