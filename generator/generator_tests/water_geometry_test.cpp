#include "testing/testing.hpp"

#include "generator/affiliation.hpp"
#include "generator/collector_water.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_source.hpp"
#include "generator/water_geometry_builder.hpp"

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
#include <fstream>
#include <map>

namespace water_geometry_generator_test
{
using P = m2::PointD;
using namespace generator;

UNIT_TEST(WaterGenerator_StandardCoverageRejectsConcaveBorderGaps)
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

UNIT_TEST(WaterGenerator_CombinedMapCoveragePreservesSeamsGapsAndHoles)
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
UNIT_TEST(WaterGenerator_RealOsmRiver)
{
  std::map<uint64_t, P> nodes;
  water::Record record;
  record.m_kind = water::Kind::Line;
  SourceReader source(GetPlatform().TestsDataPathForFile("test_data/osm/water_routing_real.osm"));
  ProcessOsmElementsFromXML(source, [&](OsmElement && element)
  {
    if (element.IsNode())
      nodes.emplace(element.m_id, mercator::FromLatLon(element.m_lat, element.m_lon));
    if (element.IsWay())
      record.m_nodes = element.m_nodes;
  });
  TEST_EQUAL(record.m_nodes.size(), 20, ());
  record.m_rings.resize(1);
  for (auto const id : record.m_nodes)
    record.m_rings.front().push_back(nodes.at(id));
  std::vector<uint8_t> bytes;
  MemWriter writer(bytes);
  auto bounds = record.GetBounds();
  bounds.Inflate(1, 1);
  water::WriteSection(writer,
                      {{P(bounds.minX(), bounds.minY()), P(bounds.maxX(), bounds.minY()),
                        P(bounds.maxX(), bounds.maxY()), P(bounds.minX(), bounds.maxY())}},
                      {record});
  water::Reader reader(ReaderPtr<::Reader>(std::make_unique<MemReader>(bytes.data(), bytes.size())));
  bounds = record.GetBounds();
  bounds.Inflate(0.1, 0.1);
  auto const geometry = routing::WaterGeometryProvider::Extract(reader, bounds, [] { return false; });
  auto const & points = record.m_rings.front();
  routing::water::Path path;
  TEST(routing::water::FindPath(geometry, points.front(), points.back(), false, [] { return false; }, path) ==
           routing::water::ResultCode::Success,
       ());
  TEST_EQUAL(path.m_points.size(), points.size(), ());
  TEST_EQUAL(path.m_landDistance, 0.0, ());
  TEST_GREATER(path.m_waterDistance, mercator::DistanceOnEarth(points.front(), points.back()), ());
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

UNIT_TEST(WaterGenerator_StandardMwmPreservesIslandsWithoutInventingRiverJoins)
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
  TEST(!data.GetMwmHandleById(id).GetValue()->m_cont.IsExist(water::kSectionTag), ());
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
  TEST_GREATER_OR_EQUAL(near.m_bounds->minX(), 0.499, ());
  TEST(FindPath(near, point, P(0.5005, 1.5), false, neverCancelled, path) == ResultCode::Success, ());
}

UNIT_TEST(WaterGenerator_StandardMapsShareWaterOnlyWithCombinedCoverage)
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
UNIT_TEST(WaterGenerator_UnrelatedCityFeaturesDoNotExhaustWaterBudget)
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

UNIT_TEST(WaterGenerator_UndergroundFeaturesAllowPortageWithoutInventingWater)
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
UNIT_TEST(WaterGenerator_PreservesSourceTopologyAndGradeSeparation)
{
  classificator::Load();
  feature::GenerateInfo info;
  auto const path = base::JoinPath(GetPlatform().WritableDir(), "water_generator_test");
  CHECK(Platform::MkDirRecursively(base::JoinPath(path, "borders")), (path));
  SCOPE_GUARD(cleanup, [&] { Platform::RmDirRecursively(path); });
  info.m_targetDir = info.m_tmpDir = info.m_intermediateDir = info.m_cacheDir = path;
  info.m_nodeStorageType = feature::GenerateInfo::NodeStorageType::File;
  std::ofstream(base::JoinPath(path, "borders", "Fixture.poly"))
      << "Fixture\n1\n-1 -1\n1 -1\n1 1\n-1 1\n-1 -1\nEND\nEND\n";
  std::ofstream(info.GetTmpFileName("Fixture", ".water_topology")) << "1n";
  {
    auto nodes = cache::CreatePointStorageWriter(info.m_nodeStorageType, info.GetCacheFileName(NODES_FILE));
    cache::IntermediateDataWriter writer(*nodes, info);
    writer.AddNode(1, 0.1, 0);
    writer.AddNode(2, 0.11, 0.01);
    writer.AddNode(3, 0.1, 0.02);
    writer.AddNode(4, 0.1, 0.2);
    writer.AddNode(5, 0.1, 0.3);
    writer.AddNode(6, 0.1, 0.4);
    writer.AddNode(7, 0.1, 0.5);
    writer.AddWay(10, WayElement(10, {1, 2, 3}));
    writer.AddWay(20, WayElement(20, {4, 5}));
    writer.AddWay(30, WayElement(30, {6, 7}));
    writer.SaveIndex();
  }
  auto river = Line(10, {P(0, 0.1), P(0.02, 0.1)}, {"waterway", "river"});
  auto culvert = Line(20, {P(0.2, 0.1), P(0.3, 0.1)}, {"waterway", "stream", "tunnel"});
  auto aqueduct = Line(30, {P(0.4, 0.1), P(0.5, 0.1)}, {"waterway", "canal"});
  {
    feature::FeaturesCollector features(info.GetTmpFileName("Fixture", ".water_features"));
    features.Collect(river);
    features.Collect(culvert);
    features.Collect(aqueduct);
  }
  {
    WaterGeometryCollector collector(info.GetIntermediateFileName(kWaterGradeSeparatedFilename));
    OsmElement source;
    source.m_type = OsmElement::EntityType::Way;
    source.m_id = 20;
    source.AddTag("tunnel", "culvert");
    collector.CollectFeature(culvert, source);
    OsmElement bridge;
    bridge.m_type = OsmElement::EntityType::Way;
    bridge.m_id = 30;
    bridge.AddTag("bridge", "aqueduct");
    collector.CollectFeature(aqueduct, bridge);
    collector.Finalize();
  }
  {
    FilesContainerW container(info.GetTargetFileName("Fixture"));
  }
  TEST(BuildWaterGeometrySection(info, "Fixture", {-2, -2, 2, 2}), ());
  FilesContainerR container(info.GetTargetFileName("Fixture"));
  water::Reader reader(container.GetReader(water::kSectionTag));
  auto const cancelled = [] { return false; };
  std::vector<water::Record> records;
  TEST(reader.Read({-0.05, 0.05, 0.05, 0.15}, 100, cancelled, records), ());
  TEST_EQUAL(records.size(), 1, ());
  TEST_EQUAL(records.front().m_nodes, (std::vector<uint64_t>{1, 2, 3}), ());
  TEST_ALMOST_EQUAL_ABS(records.front().m_rings.front()[1].x, 0.01, 1e-7, ());
  TEST_ALMOST_EQUAL_ABS(records.front().m_rings.front()[1].y, 0.11, 1e-7, ());
  auto geometry = routing::WaterGeometryProvider::Extract(reader, {-0.05, 0.05, 0.05, 0.15}, cancelled);
  routing::water::Path route;
  TEST(routing::water::FindPath(geometry, P(0, 0.1), P(0.02, 0.1), false, cancelled, route) ==
           routing::water::ResultCode::Success,
       ());
  TEST_EQUAL(route.m_points.size(), 3, ());
  TEST(reader.Read({0.15, 0.05, 0.35, 0.15}, 100, cancelled, records), ());
  TEST_EQUAL(records.size(), 1, ());
  TEST(records.front().m_kind == water::Kind::Unsupported, ());
  TEST(reader.Read({0.35, 0.05, 0.55, 0.15}, 100, cancelled, records), ());
  TEST_EQUAL(records.size(), 1, ());
  TEST(records.front().m_kind == water::Kind::Unsupported, ());
  TEST(!BuildWaterGeometrySection(info, "Fixture", {-0.5, -0.5, 0.5, 0.5}), ());

  feature::CountriesFilesIndexAffiliation normal(path, false);
  feature::CountriesFilesIndexAffiliation waterAware(path, false, true);
  auto crossing = Line(40, {P(-2, 0), P(2, 0)}, {"waterway", "river"});
  TEST(normal.GetAffiliations(crossing).empty(), ());
  TEST_EQUAL(waterAware.GetAffiliations(crossing), std::vector<std::string>{"Fixture"}, ());

  {
    cache::IntermediateDataObjectsCache objects;
    cache::IntermediateData data(objects, info);
    WaterGeometryCollector collector(base::JoinPath(path, "raw_grade"), path,
                                     std::make_shared<feature::SingleAffiliation>("Raw"), data.GetCache());
    OsmElement source;
    source.m_type = OsmElement::EntityType::Way;
    source.m_id = 20;
    source.m_nodes = {4, 5};
    source.AddTag("waterway", "stream");
    source.AddTag("tunnel", "culvert");
    collector.Collect(source);
    collector.CollectFeature(culvert, source);
    collector.Finish();
    collector.Finalize();
    auto const features = feature::ReadAllDatRawFormat(base::JoinPath(path, "Raw.water_features"));
    TEST_EQUAL(features.size(), 1, ());
    TEST(features.front().IsLine(), ());
    info.m_fileName = "Raw";
    std::ofstream(base::JoinPath(path, "borders", "Raw.poly")) << "Raw\n1\n-1 -1\n1 -1\n1 1\n-1 1\n-1 -1\nEND\nEND\n";
    std::ofstream(info.GetTmpFileName("Raw", ".water_topology")) << "1n";
    // The grade sidecar from the first collector contains this same source way.
    {
      FilesContainerW rawContainer(info.GetTargetFileName("Raw"));
    }
    TEST(BuildWaterGeometrySection(info, "Raw", {-2, -2, 2, 2}), ());
    FilesContainerR rawContainer(info.GetTargetFileName("Raw"));
    water::Reader rawReader(rawContainer.GetReader(water::kSectionTag));
    TEST(rawReader.Read({0.15, 0.05, 0.35, 0.15}, 100, cancelled, records), ());
    TEST_EQUAL(records.size(), 1, ());
    TEST(records.front().m_kind == water::Kind::Unsupported, ());
  }

  ResetWaterGeometry(path);
  TEST(!Platform::IsFileExistsByFullPath(info.GetTmpFileName("Fixture", ".water_features")), ());
  {
    base::ScopedLogAbortLevelChanger const allowExpectedError;
    TEST(!BuildWaterGeometrySection(info, "Fixture", {-2, -2, 2, 2}), ());
  }
  // A subsequent successful run without water must certify an empty section.
  std::ofstream(info.GetTmpFileName("Fixture", ".water_topology")) << "1n";
  TEST(BuildWaterGeometrySection(info, "Fixture", {-2, -2, 2, 2}), ());
  FilesContainerR emptyContainer(info.GetTargetFileName("Fixture"));
  water::Reader emptyReader(emptyContainer.GetReader(water::kSectionTag));
  TEST(emptyReader.Read({-0.05, 0.05, 0.05, 0.15}, 100, cancelled, records), ());
  TEST(records.empty(), ());
}
}  // namespace water_geometry_generator_test
