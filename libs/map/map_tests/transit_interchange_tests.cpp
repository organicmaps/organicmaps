#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "map/place_page_info.hpp"
#include "map/transit/transit_reader.hpp"

#include "drape/transit_transfer_id.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"

#include "transit/transit_graph_data.hpp"

#include "coding/files_container.hpp"

#include "geometry/mercator.hpp"

#include "base/geo_object_id.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace transit_interchange_tests
{
namespace
{
// Names of the station POIs. Station features are found by name, never by a hardcoded feature index.
std::string const kStationAName = "Station A";   // serves U3, U6
std::string const kStationBName = "Station B";   // serves U4, U5
std::string const kUnrelatedName = "Unrelated";  // no transfer

// Synthetic OSM ids used by the transit graph JSON. They are mapped to real feature ids in the fixture.
uint64_t constexpr kOsmStationA = 1001;
uint64_t constexpr kOsmStationB = 1002;
uint64_t constexpr kOsmUnrelated = 1003;

struct PoiSpec
{
  std::string m_name;
  m2::PointD m_point;
};

// Stations are placed close together; the unrelated POI is far away.
std::vector<PoiSpec> const kPois = {{kStationAName, mercator::FromLatLon(43.0, 7.0)},
                                    {kStationBName, mercator::FromLatLon(43.0001, 7.0001)},
                                    {kUnrelatedName, mercator::FromLatLon(43.5, 7.5)}};

std::string MakeGraphJson(std::string const & stops, std::string const & transfers, std::string const & lines)
{
  return R"({"stops": [)" + stops + R"(], "transfers": [)" + transfers + R"(], "lines": [)" + lines +
         R"(], "networks": [{"id": 1, "title": "Transit test"}]})";
}

std::string const kLines = R"(
  {"id": 3, "interval": 60, "network_id": 1, "number": "U3", "stop_ids": [1], "type": "subway", "color": "green"},
  {"id": 4, "interval": 60, "network_id": 1, "number": "U4", "stop_ids": [2], "type": "subway", "color": "green"},
  {"id": 5, "interval": 60, "network_id": 1, "number": "U5", "stop_ids": [2], "type": "subway", "color": "brown"},
  {"id": 6, "interval": 60, "network_id": 1, "number": "U6", "stop_ids": [1], "type": "subway", "color": "brown"})";

// Stop entries of the Odeonsplatz graph, in ascending id order.
std::string OdeonsplatzStops()
{
  return R"({"id": 1, "osm_id": ")" + std::to_string(kOsmStationA) +
         R"(", "line_ids": [3, 6], "transfer_id": 100, "point": {"x": 0.00002, "y": 0}},
     {"id": 2, "osm_id": ")" +
         std::to_string(kOsmStationB) +
         R"(", "line_ids": [4, 5], "transfer_id": 100, "point": {"x": 0.00002, "y": 0.0002}})";
}

// Odeonsplatz-like graph: stop 1 (station A, U3/U6) and stop 2 (station B, U4/U5) form transfer 100.
std::string OdeonsplatzJson()
{
  return MakeGraphJson(R"({"id": 1, "osm_id": ")" + std::to_string(kOsmStationA) +
                           R"(", "line_ids": [3, 6], "transfer_id": 100, "point": {"x": 0.00002, "y": 0}},
       {"id": 2, "osm_id": ")" +
                           std::to_string(kOsmStationB) +
                           R"(", "line_ids": [4, 5], "transfer_id": 100, "point": {"x": 0.00002, "y": 0.0002}})",
                       R"({"id": 100, "stop_ids": [1, 2], "point": {"x": 0.00002, "y": 0.0001}})", kLines);
}

// Serializes the transit graph built from |graphJson| into the MWM under construction by a TestMwmBuilder.
// Must be called before the MWM is registered, because the section is part of the file.
void WriteTransitSection(std::string const & graphJson, routing::transit::OsmIdToFeatureIdsMap const & mapping,
                         std::string const & mwmPath)
{
  routing::transit::GraphData graph;
  graph.DeserializeFromJson(graphJson, mapping);
  FilesContainerW container(mwmPath, FileWriter::OP_WRITE_EXISTING);
  auto writer = container.GetWriter(TRANSIT_FILE_TAG);
  graph.Serialize(*writer);
}

class InterchangeFixture : public generator::tests_support::TestWithCustomMwms
{
public:
  // |graphJson| is resolved against the real feature ids of the station POIs (see the constructor).
  explicit InterchangeFixture(std::string const & graphJson) : m_graphJson(graphJson)
  {
    df::VisualParams::Init(1.0, 256);

    // Pass 1: the POIs alone. Feature ids are read back by name; nothing is assumed about index order.
    auto const plainId = BuildPois("TransitInterchangePlain", nullptr);
    auto const plain = FindStations(plainId);

    // Pass 2: the same POIs plus the transit section. The osm ids of the graph are mapped to the feature
    // ids found in pass 1, and the section is written before the MWM is registered.
    routing::transit::OsmIdToFeatureIdsMap mapping;
    mapping[base::GeoObjectId(kOsmStationA)] = {plain.at(kStationAName).m_index};
    mapping[base::GeoObjectId(kOsmStationB)] = {plain.at(kStationBName).m_index};
    mapping[base::GeoObjectId(kOsmUnrelated)] = {plain.at(kUnrelatedName).m_index};
    m_id = BuildPois("TransitInterchangeTest", &mapping);
    auto const transit = FindStations(m_id);

    // The two MWMs differ in id but must index the same POIs identically, otherwise the mapping is wrong.
    for (auto const & poi : kPois)
      TEST_EQUAL(transit.at(poi.m_name).m_index, plain.at(poi.m_name).m_index, (poi.m_name));

    m_stationA = transit.at(kStationAName);
    m_stationB = transit.at(kStationBName);
    m_unrelated = transit.at(kUnrelatedName);

    m_reader = std::make_unique<TransitReadManager>(
        m_dataSource, [this](FeatureCallback const & fn, std::vector<FeatureID> const & ids)
    { m_dataSource.ReadFeatures(fn, ids); }, [this](m2::RectD const &) { return std::vector<MwmSet::MwmId>{m_id}; });
  }

  FeatureID const & StationA() const { return m_stationA; }
  FeatureID const & StationB() const { return m_stationB; }
  FeatureID const & Unrelated() const { return m_unrelated; }

  dp::TransitTransferId Transfer(uint64_t transferId) const { return {m_id, transferId}; }

  TransitReadManager & GetReader() { return *m_reader; }

  // Caches the MWM and builds the interchange index through the production viewport path.
  void LoadViewport()
  {
    m_reader->EnableTransitSchemeMode(true);
    m_reader->UpdateViewport(MakeScreen());
  }

private:
  // Builds an MWM with the station POIs. If |mapping| is set, the transit graph is written into it too.
  MwmSet::MwmId BuildPois(std::string const & name, routing::transit::OsmIdToFeatureIdsMap const * mapping)
  {
    return BuildCountry(name, [this, name, mapping](generator::tests_support::TestMwmBuilder & builder)
    {
      for (auto const & poi : kPois)
      {
        generator::tests_support::TestPOI testPoi(poi.m_point, poi.m_name, "en");
        testPoi.SetTypes({{"railway", "station", "subway"}});
        builder.Add(testPoi);
      }
      builder.Finish();
      if (mapping != nullptr)
        WriteTransitSection(m_graphJson, *mapping, GetPlatform().WritableDir() + name + ".mwm");
    });
  }

  // Returns name -> FeatureID for the POIs of |id|.
  std::map<std::string, FeatureID> FindStations(MwmSet::MwmId const & id)
  {
    std::map<std::string, FeatureID> result;
    m_dataSource.ForEachInRect([&](FeatureType & ft)
    {
      if (ft.GetID().m_mwmId != id)
        return;
      auto const name = std::string(ft.GetDefaultName());
      if (std::any_of(kPois.begin(), kPois.end(), [&name](PoiSpec const & p) { return p.m_name == name; }))
        result.emplace(name, ft.GetID());
    }, m2::RectD(mercator::FromLatLon(42.0, 6.0), mercator::FromLatLon(44.0, 8.0)), 17);
    TEST_EQUAL(result.size(), kPois.size(), ("Not all POIs found in MWM"));
    return result;
  }

  static ScreenBase MakeScreen()
  {
    ScreenBase screen;
    screen.OnSize(0, 0, 1024, 1024);
    screen.SetFromRect(m2::AnyRectD(df::GetRectForDrawScale(14, mercator::FromLatLon(43.0, 7.0))));
    return screen;
  }

  std::string m_graphJson;
  MwmSet::MwmId m_id;
  FeatureID m_stationA;
  FeatureID m_stationB;
  FeatureID m_unrelated;
  std::unique_ptr<TransitReadManager> m_reader;
};
}  // namespace

UNIT_TEST(TransitInterchange_TransferIndexesAllConstituentStations)
{
  InterchangeFixture fixture(OdeonsplatzJson());
  fixture.LoadViewport();

  auto const stations = fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100));
  TEST_EQUAL(stations.size(), 2, ());
  TEST(std::find(stations.begin(), stations.end(), fixture.StationA()) != stations.end(), ());
  TEST(std::find(stations.begin(), stations.end(), fixture.StationB()) != stations.end(), ());
  TEST(std::find(stations.begin(), stations.end(), fixture.Unrelated()) == stations.end(), ());
}

UNIT_TEST(TransitInterchange_UnknownOrInvalidTransferIsEmpty)
{
  InterchangeFixture fixture(OdeonsplatzJson());
  fixture.LoadViewport();

  TEST(fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(999)).empty(), ());
  TEST(fixture.GetReader().GetTransferStationFeatures(dp::TransitTransferId{}).empty(), ());
}

UNIT_TEST(TransitInterchange_NotIndexedBeforeViewport)
{
  InterchangeFixture fixture(OdeonsplatzJson());
  TEST(fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100)).empty(), ());
}

UNIT_TEST(TransitInterchange_ClearDropsIndex)
{
  InterchangeFixture fixture(OdeonsplatzJson());
  fixture.LoadViewport();
  TEST(!fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100)).empty(), ());

  fixture.GetReader().Clear();
  TEST(fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100)).empty(), ());
}

UNIT_TEST(TransitInterchange_DisablingSchemeDropsIndex)
{
  InterchangeFixture fixture(OdeonsplatzJson());
  fixture.LoadViewport();
  TEST(!fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100)).empty(), ());

  fixture.GetReader().EnableTransitSchemeMode(false);
  TEST(fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100)).empty(), ());
}

UNIT_TEST(TransitInterchange_BlockingSchemeDropsIndex)
{
  InterchangeFixture fixture(OdeonsplatzJson());
  fixture.LoadViewport();
  TEST(!fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100)).empty(), ());

  fixture.GetReader().BlockTransitSchemeMode(true);
  TEST(fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100)).empty(), ());
}

UNIT_TEST(TransitInterchange_OrderOfMembersDoesNotChangeResult)
{
  // Stop entries must be in ascending id order (delta-encoded by the serializer), so only the transfer's
  // stop_ids list is reversed here.
  InterchangeFixture fixture(MakeGraphJson(
      OdeonsplatzStops(), R"({"id": 100, "stop_ids": [2, 1], "point": {"x": 0.00002, "y": 0.0001}})", kLines));
  fixture.LoadViewport();

  auto const stations = fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100));
  TEST_EQUAL(stations.size(), 2, ());
  TEST(std::is_sorted(stations.begin(), stations.end()), ());
  TEST(std::find(stations.begin(), stations.end(), fixture.StationA()) != stations.end(), ());
  TEST(std::find(stations.begin(), stations.end(), fixture.StationB()) != stations.end(), ());
}

UNIT_TEST(TransitInterchange_DuplicateStationIsReportedOnce)
{
  // Both stops of the transfer resolve to station A, so the interchange has a single station feature.
  InterchangeFixture fixture(MakeGraphJson(
      R"({"id": 1, "osm_id": ")" + std::to_string(kOsmStationA) +
          R"(", "line_ids": [3], "transfer_id": 100, "point": {"x": 0.00002, "y": 0}},
       {"id": 2, "osm_id": ")" +
          std::to_string(kOsmStationA) + R"(", "line_ids": [4], "transfer_id": 100, "point": {"x": 0.00002, "y": 0}})",
      R"({"id": 100, "stop_ids": [1, 2], "point": {"x": 0.00002, "y": 0}})", kLines));
  fixture.LoadViewport();

  auto const stations = fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100));
  TEST_EQUAL(stations.size(), 1, ());
  TEST_EQUAL(stations.front(), fixture.StationA(), ());
}

UNIT_TEST(TransitInterchange_UnresolvedStopsAreSkipped)
{
  // Stop 2 has no feature (its osm id is not in the mapping) and stop 77 does not exist.
  InterchangeFixture fixture(
      MakeGraphJson(R"({"id": 1, "osm_id": ")" + std::to_string(kOsmStationA) +
                        R"(", "line_ids": [3], "transfer_id": 100, "point": {"x": 0.00002, "y": 0}},
       {"id": 2, "osm_id": "999999", "line_ids": [4], "transfer_id": 100, "point": {"x": 0.00002, "y": 0.0001}})",
                    R"({"id": 100, "stop_ids": [1, 2, 77], "point": {"x": 0.00002, "y": 0}})", kLines));
  fixture.LoadViewport();

  auto const stations = fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100));
  TEST_EQUAL(stations.size(), 1, ());
  TEST_EQUAL(stations.front(), fixture.StationA(), ());
}

UNIT_TEST(TransitInterchange_OrdinaryStationIsNotAnInterchange)
{
  // Station A has no transfer. It must not be reported as an interchange.
  InterchangeFixture fixture(MakeGraphJson(R"({"id": 1, "osm_id": ")" + std::to_string(kOsmStationA) +
                                               R"(", "line_ids": [3, 6], "point": {"x": 0.00002, "y": 0}},
       {"id": 2, "osm_id": ")" + std::to_string(kOsmStationB) +
                                               R"(", "line_ids": [4, 5], "point": {"x": 0.00002, "y": 0.0002}})",
                                           "", kLines));
  fixture.LoadViewport();

  TEST(fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100)).empty(), ());
}

UNIT_TEST(TransitInterchange_UnrelatedNearbyStationStaysOut)
{
  // Stop 1 (station A) is in transfer 100 with stop 2 (station B). Stop 3 (unrelated) is a separate stop on
  // another line with no transfer and must not join the interchange.
  InterchangeFixture fixture(MakeGraphJson(
      R"({"id": 1, "osm_id": ")" + std::to_string(kOsmStationA) +
          R"(", "line_ids": [3, 6], "transfer_id": 100, "point": {"x": 0.00002, "y": 0}},
       {"id": 2, "osm_id": ")" +
          std::to_string(kOsmStationB) +
          R"(", "line_ids": [4, 5], "transfer_id": 100, "point": {"x": 0.00002, "y": 0.0002}},
       {"id": 3, "osm_id": ")" +
          std::to_string(kOsmUnrelated) + R"(", "line_ids": [7], "point": {"x": 0.5, "y": 0.5}})",
      R"({"id": 100, "stop_ids": [1, 2], "point": {"x": 0.00002, "y": 0.0001}})",
      kLines +
          R"(, {"id": 7, "interval": 60, "network_id": 1, "number": "U7", "stop_ids": [3], "type": "subway", "color": "red"})"));
  fixture.LoadViewport();

  auto const stations = fixture.GetReader().GetTransferStationFeatures(fixture.Transfer(100));
  TEST_EQUAL(stations.size(), 2, ());
  TEST(std::find(stations.begin(), stations.end(), fixture.Unrelated()) == stations.end(), ());
}

}  // namespace transit_interchange_tests
