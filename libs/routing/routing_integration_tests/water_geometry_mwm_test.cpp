#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/scales.hpp"
#include "indexer/water_geometry.hpp"

#include "routing/water_geometry_provider.hpp"
#include "storage/country_info_getter.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/geometry_coding.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <algorithm>

namespace water_geometry_mwm_test
{
using P = m2::PointD;
using namespace generator;

UNIT_TEST(WaterMwm_StandardCoverageRejectsConcaveBorderGaps)
{
  auto const path = base::JoinPath(GetPlatform().WritableDir(), "water_borders_test.bin");
  SCOPE_GUARD(cleanup, [&] { base::DeleteFileX(path); });
  {
    FilesContainerW borders(path);
    {
      auto writer = borders.GetWriter(PACKED_POLYGONS_INFO_TAG);
      rw::Write(*writer, std::vector<storage::CountryDef>{{"Concave", {-3, -3, 3, 3}}});
    }
    auto writer = borders.GetWriter("0");
    WriteVarUint(*writer, 1u);
    serial::SaveOuterPath(
        std::vector<P>{{-3, -3}, {3, -3}, {3, 3}, {1, 3}, {1, 0}, {-1, 0}, {-1, 3}, {-3, 3}, {-3, -3}},
        serial::GeometryCodingParams(), *writer);
  }
  class BorderReader : public storage::CountryInfoReader
  {
  public:
    explicit BorderReader(std::string const & path) : CountryInfoReader(GetPlatform().GetReader(path)) {}
  } reader(path);
  TEST(reader.IsRectCoveredByRegion("Concave", {-0.5, -2, 0.5, -1}), ());
  TEST(!reader.IsRectCoveredByRegion("Concave", {-2, 1, 2, 2}), ());
  TEST(!reader.IsRectCoveredByRegion("Missing", {-0.5, -2, 0.5, -1}), ());
  TEST(!reader.IsRectCoveredByRegion("Concave", {-3, -2, -2, -1}), ());
}

UNIT_TEST(WaterMwm_CombinedMapCoveragePreservesSeamsGapsAndHoles)
{
  auto const path = base::JoinPath(GetPlatform().WritableDir(), "water_combined_borders_test.bin");
  SCOPE_GUARD(cleanup, [&] { base::DeleteFileX(path); });
  std::vector<std::vector<P>> const rings = {{{-3, -3}, {0, -3}, {0, 3}, {-3, 3}},
                                             {{0, -3}, {3, -3}, {3, 3}, {0, 3}},
                                             {{0.000005, -3}, {3, -3}, {3, 3}, {0.000005, 3}},
                                             {{-3, -3}, {3, -3}, {3, -1}, {-3, -1}},
                                             {{-3, 1}, {3, 1}, {3, 3}, {-3, 3}},
                                             {{-3, -1}, {-1, -1}, {-1, 1}, {-3, 1}},
                                             {{1, -1}, {3, -1}, {3, 1}, {1, 1}}};
  std::vector<std::string> const names = {"Left", "Right", "RightGap", "Bottom", "Top", "West", "East"};
  {
    FilesContainerW borders(path);
    std::vector<storage::CountryDef> definitions;
    for (auto const & name : names)
      definitions.push_back({name, {-3, -3, 3, 3}});
    {
      auto writer = borders.GetWriter(PACKED_POLYGONS_INFO_TAG);
      rw::Write(*writer, definitions);
    }
    for (size_t i = 0; i < rings.size(); ++i)
    {
      auto writer = borders.GetWriter(std::to_string(i));
      WriteVarUint(*writer, 1u);
      serial::SaveOuterPath(rings[i], serial::GeometryCodingParams(), *writer);
    }
  }
  class BorderReader : public storage::CountryInfoReader
  {
  public:
    explicit BorderReader(std::string const & path) : CountryInfoReader(GetPlatform().GetReader(path)) {}
  } reader(path);
  m2::RectD const query(-0.5, -0.5, 0.5, 0.5);
  TEST(!reader.IsRectCoveredByRegion("Left", query), ());
  TEST(!reader.IsRectCoveredByRegion("Right", query), ());
  TEST(reader.IsRectCoveredByRegions({"Left", "Right"}, query), ());
  TEST(reader.IsRectCoveredByRegions({"Right", "Left", "Left"}, query), ());
  TEST(!reader.IsRectCoveredByRegions({"Left", "RightGap"}, query), ());
  TEST(!reader.IsRectCoveredByRegions({"Left", "Missing"}, query), ());
  TEST(!reader.IsRectCoveredByRegions({}, query), ());
  TEST(!reader.IsRectCoveredByRegions({"Bottom", "Top", "West", "East"}, {-2, -2, 2, 2}), ());
  TEST(reader.IsRectCoveredByRegions({"Bottom", "Top", "West", "East"}, {-2.5, -2.5, -1.5, -1.5}), ());
  TEST(!reader.IsRectCoveredByRegions({"Left", "Right"}, {-3, -1, 1, 1}), ());
}
feature::FeatureBuilder Line(uint64_t id, std::vector<P> points, std::initializer_list<char const *> type)
{
  feature::FeatureBuilder feature;
  feature.AssignPoints(std::move(points));
  feature.SetLinear();
  feature.SetOsmId(base::MakeOsmWay(id));
  feature.AddType(classif().GetTypeByPath(type));
  return feature;
}

UNIT_TEST(WaterMwm_StandardMwmPreservesIslandsWithoutInventingRiverJoins)
{
  classificator::Load();
  auto const directory = base::JoinPath(GetPlatform().WritableDir(), "water_standard_test");
  CHECK(Platform::MkDirRecursively(directory), ());
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(directory); });
  platform::LocalCountryFile file(directory, platform::CountryFile("WaterStandard"), 0);
  {
    tests_support::TestMwmBuilder builder(file, feature::DataHeader::MapType::Country);
    feature::FeatureBuilder lake;
    lake.SetArea();
    lake.AddType(classif().GetTypeByPath({"natural", "water", "lake"}));
    lake.AddPolygon({P(0, 0), P(3, 0), P(3, 3), P(0, 3), P(0, 0)});
    lake.AddPolygon({P(1, 1), P(1, 2), P(2, 2), P(2, 1), P(1, 1)});
    TEST(builder.Add(lake), ());
    auto river = Line(1, {P(4, 0), P(4, 1), P(5, 1)}, {"waterway", "river"});
    auto touching = Line(2, {P(5, 1), P(6, 1)}, {"waterway", "river"});
    auto crossing = Line(3, {P(3.5, 0.5), P(4.5, 0.5)}, {"waterway", "river"});
    TEST(builder.Add(river), ());
    TEST(builder.Add(touching), ());
    TEST(builder.Add(crossing), ());
    // Standard display data can lose bridge=aqueduct without layer.
    auto aqueduct = Line(4, {P(-1, 0.5), P(4, 0.5)}, {"waterway", "canal"});
    aqueduct.GetParams().layer = 1;
    TEST(builder.Add(aqueduct), ());
    builder.Finish();
  }
  FrozenDataSource data;
  auto const [id, status] = data.RegisterMap(file);
  TEST(status == MwmSet::RegResult::Success, ());
  TEST(!data.GetMwmHandleById(id).GetValue()->m_cont.IsExist(::water::kSectionTag), ());
  auto const neverCancelled = [] { return false; };
  auto const geometry = routing::WaterGeometryProvider::ExtractFeatures(data, id, {-2, -2, 8, 5}, neverCancelled);
  TEST(geometry.m_complete, ());
  TEST(!geometry.m_hasSourceTopology, ());
  routing::water::Path path;
  using routing::water::ResultCode;
  TEST(FindPath(geometry, P(0.5, 1.5), P(2.5, 1.5), false, neverCancelled, path) == ResultCode::Success, ());
  TEST_GREATER(path.m_points.size(), 2, ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST(FindPath(geometry, P(1.5, 1.5), P(2.5, 1.5), false, neverCancelled, path) == ResultCode::OutsideWater, ());
  auto const aqueduct = std::find_if(geometry.m_lines.begin(), geometry.m_lines.end(),
                                     [](auto const & line) { return line.front().m_point.x < 0; });
  TEST(aqueduct != geometry.m_lines.end(), ());
  TEST(FindPath(geometry, aqueduct->front().m_point, P(0.5, 0.8), false, neverCancelled, path) == ResultCode::Success,
       ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  auto const river = std::find_if(geometry.m_lines.begin(), geometry.m_lines.end(),
                                  [](auto const & line) { return line.size() == 3; });
  TEST(river != geometry.m_lines.end(), ());
  auto const start = river->front().m_point;
  auto const finish = (river->back().m_point + (*river)[1].m_point) * 0.5;
  TEST(FindPath(geometry, start, finish, false, neverCancelled, path) == ResultCode::Success, ());
  TEST_GREATER(path.m_waterDistance, mercator::DistanceOnEarth(start, finish), ());
  for (auto const & line : geometry.m_lines)
    if (&line != &*river && &line != &*aqueduct)
    {
      TEST(FindPath(geometry, start, line.back().m_point, false, neverCancelled, path) == ResultCode::Success, ());
      TEST_GREATER(path.m_landDistance, 0.0, ());
      TEST_EQUAL(std::count(path.m_landGaps.begin(), path.m_landGaps.end(), true), 1, ());
      auto const isEndpoint = [&](auto const & point)
      {
        return std::any_of(geometry.m_lines.begin(), geometry.m_lines.end(), [&](auto const & candidate)
        {
          return (candidate.front().m_mayTransfer && candidate.front().m_point == point) ||
                 (candidate.back().m_mayTransfer && candidate.back().m_point == point);
        });
      };
      for (size_t i = 0; i < path.m_landGaps.size(); ++i)
        if (path.m_landGaps[i])
        {
          TEST(isEndpoint(path.m_points[i]), ());
          TEST(isEndpoint(path.m_points[i + 1]), ());
        }
    }
  TEST(!routing::WaterGeometryProvider::ExtractFeatures(data, id, {-2, -2, 8, 5}, [] { return true; }).m_complete, ());
  routing::WaterGeometryProvider withoutCoverage(data);
  routing::RouterDelegate delegate;
  routing::Checkpoints const checkpoints(P(0.5, 1.5), P(2.5, 1.5));
  TEST(!withoutCoverage.GetGeometry(checkpoints, delegate).m_complete, ());
  routing::WaterGeometryProvider covered(data, [](auto const &, auto const &) { return true; });
  TEST(covered.GetGeometry(checkpoints, delegate).m_complete, ());
  m2::RectD pointBounds;
  routing::WaterGeometryProvider pointProvider(data, [&](auto const &, auto const & bounds)
  {
    pointBounds = bounds;
    return true;
  });
  auto const point = P(0.5, 1.5);
  auto const local = pointProvider.GetGeometry(routing::Checkpoints(point, point), delegate);
  TEST_LESS(mercator::DistanceOnEarth(pointBounds.LeftBottom(), pointBounds.RightTop()), 150.0, ());
  TEST(FindPath(local, point, point, false, neverCancelled, path) == ResultCode::Success, ());
  routing::WaterGeometryProvider nearBorder(data,
                                            [](auto const &, auto const & bounds) { return bounds.minX() >= 0.499; });
  auto const near = nearBorder.GetGeometry(routing::Checkpoints(point, P(0.5005, 1.5)), delegate);
  TEST(near.m_complete, ());
  TEST(near.m_bounds.has_value(), ());
  TEST(!near.m_unknownAreas.empty(), ());
  TEST(FindPath(near, P(0.4989, 1.5), P(0.5005, 1.5), false, neverCancelled, path) == ResultCode::Unavailable, ());
  TEST(FindPath(near, point, P(0.5005, 1.5), false, neverCancelled, path) == ResultCode::Success, ());
  auto const close = nearBorder.GetGeometry(routing::Checkpoints(P(0.49901, 1.5), P(0.5005, 1.5)), delegate);
  TEST(close.m_complete, ());
  auto const entry = routing::water::FindApproachPoint(close, P(0.49901, 1.5), neverCancelled);
  TEST(entry.has_value(), ());
}

UNIT_TEST(WaterMwm_PreparedWaterSectionRemainsReadable)
{
  classificator::Load();
  auto const directory = base::JoinPath(GetPlatform().WritableDir(), "water_prepared_section_test");
  CHECK(Platform::MkDirRecursively(directory), ());
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(directory); });
  platform::LocalCountryFile file(directory, platform::CountryFile("WaterPrepared"), 0);
  {
    tests_support::TestMwmBuilder builder(file, feature::DataHeader::MapType::Country);
    auto river = Line(1, {P(0, 0.5), P(1, 0.5)}, {"waterway", "river"});
    TEST(builder.Add(river), ());
    builder.Finish();
  }
  // Represent an already prepared experimental map without a generator extension.
  {
    FilesContainerW container(file.GetPath(MapFileType::Map), FileWriter::OP_WRITE_EXISTING);
    auto writer = container.GetWriter(::water::kSectionTag);
    ::water::WriteSection(*writer, {{P(-2, -2), P(3, -2), P(3, 3), P(-2, 3)}},
                          {{::water::Kind::Line, {{P(0, 0.5), P(0.5, 0.7), P(1, 0.5)}}, {1, 2, 3}}});
  }
  FrozenDataSource data;
  auto const [id, status] = data.RegisterMap(file);
  TEST(status == MwmSet::RegResult::Success, ());
  routing::WaterGeometryProvider provider(data);
  routing::RouterDelegate delegate;
  routing::Checkpoints const checkpoints(P(0, 0.5), P(1, 0.5));
  auto const geometry = provider.GetGeometry(checkpoints, delegate);
  TEST(geometry.m_complete, ());
  TEST(geometry.m_hasSourceTopology, ());
  TEST_EQUAL(geometry.m_lines.size(), 1, ());
  TEST_EQUAL(geometry.m_lines.front().size(), 3, ());
  routing::water::Path path;
  TEST(FindPath(geometry, checkpoints.GetStart(), checkpoints.GetFinish(), false, [] { return false; }, path) ==
           routing::water::ResultCode::Success,
       ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST_GREATER(path.m_waterDistance, mercator::DistanceOnEarth(checkpoints.GetStart(), checkpoints.GetFinish()), ());
}

UNIT_TEST(WaterMwm_StandardMapsShareWaterOnlyWithCombinedCoverage)
{
  classificator::Load();
  auto const directory = base::JoinPath(GetPlatform().WritableDir(), "water_multi_map_test");
  CHECK(Platform::MkDirRecursively(directory), ());
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(directory); });
  platform::LocalCountryFile left(directory, platform::CountryFile("WaterLeft"), 0);
  platform::LocalCountryFile right(directory, platform::CountryFile("WaterRight"), 0);
  auto const build = [&](platform::LocalCountryFile & file, double from, double to)
  {
    tests_support::TestMwmBuilder builder(file, feature::DataHeader::MapType::Country);
    feature::FeatureBuilder surface;
    surface.SetArea();
    surface.AddType(classif().GetTypeByPath({"natural", "water", "lake"}));
    surface.AddPolygon({P(from, 0), P(to, 0), P(to, 1), P(from, 1), P(from, 0)});
    TEST(builder.Add(surface), ());
    builder.Finish();
  };
  build(left, 0, 1);
  build(right, 1, 2);
  FrozenDataSource data;
  auto const [leftId, leftStatus] = data.RegisterMap(left);
  TEST(leftStatus == MwmSet::RegResult::Success, ());
  auto const singleCoverage = [](std::string const & country, m2::RectD const & bounds)
  { return (country == "WaterLeft" ? m2::RectD(0, -1, 1, 2) : m2::RectD(1, -1, 2, 2)).IsRectInside(bounds); };
  auto const combinedCoverage = [](std::vector<std::string> const & countries, m2::RectD const & bounds)
  {
    return countries.size() == 2 && std::find(countries.begin(), countries.end(), "WaterLeft") != countries.end() &&
           std::find(countries.begin(), countries.end(), "WaterRight") != countries.end() &&
           m2::RectD(0, -1, 2, 2).IsRectInside(bounds);
  };
  routing::WaterGeometryProvider provider(data, singleCoverage, combinedCoverage);
  routing::RouterDelegate delegate;
  routing::Checkpoints const checkpoints(P(0.5, 0.5), P(1.5, 0.5));
  TEST(!provider.GetGeometry(checkpoints, delegate).m_complete, ());
  auto const [rightId, rightStatus] = data.RegisterMap(right);
  TEST(rightStatus == MwmSet::RegResult::Success, ());
  routing::WaterGeometryProvider noCombinedProof(data, singleCoverage);
  TEST(!noCombinedProof.GetGeometry(checkpoints, delegate).m_complete, ());
  auto const geometry = provider.GetGeometry(checkpoints, delegate);
  TEST(geometry.m_complete, ());
  routing::water::Path path;
  TEST(FindPath(geometry, checkpoints.GetPoints()[0], checkpoints.GetPoints()[1], false, [] { return false; }, path) ==
           routing::water::ResultCode::Success,
       ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST_GREATER(path.m_waterDistance, 0.0, ());
  // Use the stored edge: MWM quantization can move the source coordinate.
  auto const fullLeft =
      routing::WaterGeometryProvider::ExtractFeatures(data, leftId, {-0.1, -0.1, 1.2, 1.1}, [] { return false; });
  double edge = 0.0;
  for (auto const & triangle : fullLeft.m_triangles)
    for (auto const & point : triangle)
      edge = std::max(edge, point.x);
  TEST_GREATER(edge, 0.9, ());
  // Boundary clipping produces flat triangles even from valid water polygons.
  auto const edgeOnly =
      routing::WaterGeometryProvider::ExtractFeatures(data, leftId, {edge, 0.1, 1.2, 0.9}, [] { return false; });
  TEST(edgeOnly.m_complete, ());
  TEST(edgeOnly.m_triangles.empty(), ());
  auto const thin = routing::WaterGeometryProvider::ExtractFeatures(data, leftId, {edge - 0.001, 0.1, 1.2, 0.9},
                                                                    [] { return false; });
  TEST(thin.m_complete, ());
  TEST(!thin.m_triangles.empty(), ());
  TEST(FindPath(thin, P(edge - 0.0005, 0.2), P(edge - 0.0005, 0.8), false, [] { return false; }, path) ==
           routing::water::ResultCode::Success,
       ());
  // An uncovered corner does not invalidate a route through certified water.
  routing::WaterGeometryProvider partial(data, singleCoverage, [&](auto const & names, auto const & bounds)
  { return combinedCoverage(names, bounds) && !(bounds.minX() < 0.9 && bounds.maxY() > 0.55); });
  auto const known = partial.GetGeometry(routing::Checkpoints(P(0.5, 0.5), P(1.5, 0.6)), delegate);
  TEST(known.m_complete, ());
  TEST(!known.m_unknownAreas.empty(), ());
  TEST(FindPath(known, P(0.5, 0.5), P(1.5, 0.6), false, [] { return false; }, path) ==
           routing::water::ResultCode::Success,
       ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST(FindPath(known, P(0.7, 0.6), P(1.5, 0.6), false, [] { return false; }, path) ==
           routing::water::ResultCode::Unavailable,
       ());
  // Even with both checkpoints on one map, a detour may use its neighbor.
  auto const nearSeam = provider.GetGeometry(routing::Checkpoints(P(0.997, 0.5), P(0.999, 0.5)), delegate);
  TEST(nearSeam.m_complete, ());
  TEST(nearSeam.m_bounds.has_value(), ());
  TEST_GREATER(nearSeam.m_bounds->maxX(), 1.0, ());
  TEST(!routing::WaterGeometryProvider::ExtractFeatures(data, std::vector<MwmSet::MwmId>{leftId, rightId},
                                                        {0.4, 0.4, 1.6, 0.6}, [] { return true; })
            .m_complete,
       ());
}
UNIT_TEST(WaterMwm_UnrelatedCityFeaturesDoNotExhaustWaterBudget)
{
  classificator::Load();
  auto const directory = base::JoinPath(GetPlatform().WritableDir(), "water_city_test");
  CHECK(Platform::MkDirRecursively(directory), ());
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(directory); });
  platform::LocalCountryFile file(directory, platform::CountryFile("WaterCity"), 0);
  {
    tests_support::TestMwmBuilder builder(file, feature::DataHeader::MapType::Country);
    auto canal = Line(1, {P(0, 0), P(0, 0.0002)}, {"waterway", "canal"});
    TEST(builder.Add(canal), ());
    for (uint64_t i = 0; i < 17000; ++i)
    {
      feature::FeatureBuilder bench;
      bench.SetCenter(P((i % 129) * 0.000001, (i / 129) * 0.000001));
      bench.SetOsmId(base::GeoObjectId(base::GeoObjectId::Type::ObsoleteOsmNode, i + 1));
      bench.AddType(classif().GetTypeByPath({"amenity", "bench"}));
      TEST(builder.Add(bench), ());
    }
    builder.Finish();
  }
  FrozenDataSource data;
  auto const [id, status] = data.RegisterMap(file);
  TEST(status == MwmSet::RegResult::Success, ());
  m2::RectD const bounds(-0.001, -0.001, 0.001, 0.001);
  size_t count = 0;
  data.ForEachInRectForMWM([&](FeatureType &) { ++count; }, bounds, scales::GetUpperScale(), id);
  TEST_GREATER(count, 16384, ());
  auto const geometry = routing::WaterGeometryProvider::ExtractFeatures(data, id, bounds, [] { return false; });
  TEST(geometry.m_complete, ());
  TEST_EQUAL(geometry.m_lines.size(), 1, ());
  routing::water::Path path;
  auto const & line = geometry.m_lines.front();
  TEST(FindPath(geometry, line.front().m_point, line.back().m_point, false, [] { return false; }, path) ==
           routing::water::ResultCode::Success,
       ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
}

UNIT_TEST(WaterMwm_LargeLineNetworkUsesGraphCapacity)
{
  classificator::Load();
  auto const directory = base::JoinPath(GetPlatform().WritableDir(), "water_large_line_test");
  CHECK(Platform::MkDirRecursively(directory), ());
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(directory); });
  platform::LocalCountryFile file(directory, platform::CountryFile("WaterLargeLines"), 0);
  size_t constexpr count = 17000;
  {
    tests_support::TestMwmBuilder builder(file, feature::DataHeader::MapType::Country);
    for (size_t i = 0; i < count; ++i)
    {
      double const x = i * 0.0001;
      auto canal = Line(i + 1, {P(x, 0), P(x, 0.01)}, {"waterway", "canal"});
      TEST(builder.Add(canal), ());
    }
    builder.Finish();
  }
  FrozenDataSource data;
  auto const [id, status] = data.RegisterMap(file);
  TEST(status == MwmSet::RegResult::Success, ());
  auto const geometry =
      routing::WaterGeometryProvider::ExtractFeatures(data, id, {-0.01, -0.01, 2, 0.02}, [] { return false; });
  TEST(geometry.m_complete, ());
  TEST_EQUAL(geometry.m_lines.size(), count, ());
  size_t vertices = 0;
  for (auto const & line : geometry.m_lines)
    vertices += line.size();
  TEST_EQUAL(vertices, 2 * count, ());
  TEST_LESS(vertices, routing::water::kMaxGraphVertices, ());
}

UNIT_TEST(WaterMwm_UndergroundFeaturesAllowPortageWithoutInventingWater)
{
  classificator::Load();
  auto const directory = base::JoinPath(GetPlatform().WritableDir(), "water_negative_layer_test");
  CHECK(Platform::MkDirRecursively(directory), ());
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(directory); });
  platform::LocalCountryFile file(directory, platform::CountryFile("WaterNegativeLayer"), 0);
  {
    tests_support::TestMwmBuilder builder(file, feature::DataHeader::MapType::Country);
    feature::FeatureBuilder surface;
    surface.SetArea();
    surface.AddType(classif().GetTypeByPath({"natural", "water", "river"}));
    surface.AddPolygon({P(0, 0), P(0.01, 0), P(0.01, 0.01), P(0, 0.01), P(0, 0)});
    surface.GetParams().layer = -1;
    TEST(builder.Add(surface), ());
    auto river = Line(1, {P(0.02, 0), P(0.02, 0.01)}, {"waterway", "river"});
    river.GetParams().layer = -1;
    TEST(builder.Add(river), ());
    auto surfaceRiver = Line(2, {P(0.03, 0), P(0.03, 0.01)}, {"waterway", "river"});
    TEST(builder.Add(surfaceRiver), ());
    builder.Finish();
  }
  FrozenDataSource data;
  auto const [id, status] = data.RegisterMap(file);
  TEST(status == MwmSet::RegResult::Success, ());
  auto const geometry =
      routing::WaterGeometryProvider::ExtractFeatures(data, id, {-0.005, -0.005, 0.035, 0.015}, [] { return false; });
  TEST(geometry.m_complete, ());
  TEST_EQUAL(geometry.m_lines.size(), 1, ());
  TEST(geometry.m_triangles.empty(), ());
  TEST(geometry.m_unknownAreas.empty(), ());
  TEST_EQUAL(geometry.m_portageAreas.size(), 2, ());
  TEST(std::any_of(geometry.m_portageAreas.begin(), geometry.m_portageAreas.end(),
                   [](auto const & rect) { return rect.IsPointInside(P(0.005, 0.005)); }),
       ());
  TEST(std::any_of(geometry.m_portageAreas.begin(), geometry.m_portageAreas.end(),
                   [](auto const & rect) { return rect.IsIntersect({0.01999, 0.00499, 0.02001, 0.00501}); }),
       ());
}
UNIT_TEST(WaterMwm_SurfaceGroupsPreserveOverlapContactsAndIslands)
{
  classificator::Load();
  auto const directory = base::JoinPath(GetPlatform().WritableDir(), "water_surface_groups_test");
  CHECK(Platform::MkDirRecursively(directory), ());
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(directory); });
  platform::LocalCountryFile file(directory, platform::CountryFile("WaterSurfaceGroups"), 0);
  {
    tests_support::TestMwmBuilder builder(file, feature::DataHeader::MapType::Country);
    auto const area = [&](std::vector<std::vector<P>> const & rings)
    {
      feature::FeatureBuilder feature;
      feature.SetArea();
      feature.AddType(classif().GetTypeByPath({"natural", "water", "lake"}));
      for (auto ring : rings)
        feature.AddPolygon(std::move(ring));
      TEST(builder.Add(feature), ());
    };
    area({{P(0, 0), P(3, 0), P(3, 3), P(0, 3)}, {P(1, 1), P(1, 2), P(2, 2), P(2, 1)}});
    area({{P(2.5, 0), P(4, 0), P(4, 3), P(2.5, 3)}});
    area({{P(4, 0), P(5, 0), P(5, 3), P(4, 3)}});
    for (size_t i = 0; i < 40; ++i)
    {
      double const x = i * 0.2;
      area({{P(x, 4), P(x + 0.1, 4), P(x + 0.1, 4.1), P(x, 4.1)}});
    }
    builder.Finish();
  }
  FrozenDataSource data;
  auto const [id, status] = data.RegisterMap(file);
  TEST(status == MwmSet::RegResult::Success, ());
  auto const geometry = routing::WaterGeometryProvider::ExtractFeatures(data, id, {-1, -1, 9, 5}, [] { return false; });
  TEST(geometry.m_complete, ());
  routing::water::Path path;
  TEST(FindPath(geometry, P(0.5, 1.5), P(4.5, 1.5), false, [] { return false; }, path) ==
           routing::water::ResultCode::Success,
       ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST_GREATER(path.m_waterDistance, mercator::DistanceOnEarth(P(0.5, 1.5), P(4.5, 1.5)), ());
  TEST(FindPath(geometry, P(1.5, 1.5), P(4.5, 1.5), false, [] { return false; }, path) ==
           routing::water::ResultCode::OutsideWater,
       ());
  TEST(FindPath(geometry, P(0.025, 4.025), P(0.075, 4.075), false, [] { return false; }, path) ==
           routing::water::ResultCode::Success,
       ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST(FindPath(geometry, P(0.025, 4.025), P(0.5, 1.5), false, [] { return false; }, path, {}, true) ==
           routing::water::ResultCode::Unavailable,
       ());
}
}  // namespace water_geometry_mwm_test
