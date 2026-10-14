#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "map/transit/transit_display.hpp"

#include "drape_frontend/route_renderer.hpp"

#include "transit/transit_graph_data.hpp"

#include "coding/files_container.hpp"

#include "base/strings_bundle.hpp"

#include <algorithm>

namespace transit_rendering_tests
{
namespace
{
class Fixture : public generator::tests_support::TestWithCustomMwms
{
public:
  explicit Fixture(std::string const & graphJson = R"({
        "stops": [
          {"id": 1, "osm_id": "1", "line_ids": [1], "point": {"x": 0.00002, "y": 0}},
          {"id": 2, "osm_id": "2", "line_ids": [1], "point": {"x": 0.00003, "y": 0}}
        ],
        "lines": [{"id": 1, "interval": 60, "network_id": 1, "number": "1", "stop_ids": [1, 2],
                   "type": "subway", "color": "red"}],
        "networks": [{"id": 1, "title": "Transit test"}]
      })")
    : m_reader(m_dataSource, [this](FeatureCallback const & fn, std::vector<FeatureID> const & ids)
  { m_dataSource.ReadFeatures(fn, ids); }, [](m2::RectD const &) { return std::vector<MwmSet::MwmId>(); })
  {
    m_strings.SetString("core_entrance", "Entrance");
    m_strings.SetString("core_exit", "Exit");
    m_id = BuildCountry("TransitRenderingTest", [&graphJson](generator::tests_support::TestMwmBuilder & builder)
    {
      generator::tests_support::TestPOI poi({0.00001, 0}, "Gate", "en");
      poi.SetTypes({{"railway", "subway_entrance"}});
      builder.Add(poi);
      builder.Finish();

      routing::transit::GraphData graph;
      graph.DeserializeFromJson(graphJson, {});
      FilesContainerW container(GetPlatform().WritableDir() + "TransitRenderingTest.mwm",
                                FileWriter::OP_WRITE_EXISTING);
      auto writer = container.GetWriter(TRANSIT_FILE_TAG);
      graph.Serialize(*writer);
    });
  }

  TransitRouteDisplay MakeDisplay()
  {
    return TransitRouteDisplay(m_reader, [this](routing::NumMwmId) { return m_id; },
                               [this]() -> StringsBundle const & { return m_strings; }, nullptr, m_symbols);
  }

private:
  MwmSet::MwmId m_id;
  TransitReadManager m_reader;
  StringsBundle m_strings;
  std::map<std::string, m2::PointF> const m_symbols;
};

std::vector<routing::RouteSegment> MakeSegments()
{
  std::vector<m2::PointD> const points = {
      {0.00001, 0}, {0.00002, 0}, {0.00003, 0}, {0.00004, 0.00002}, {0.00005, 0.00002}};
  std::vector<double> const times = {10, 70, 190, 250, 260};
  std::vector<routing::RouteSegment> segments;
  for (size_t i = 0; i < points.size(); ++i)
  {
    segments.emplace_back(routing::Segment(0, 0, i, true), routing::turns::TurnItem(),
                          geometry::PointWithAltitude(points[i], 0), routing::RouteSegment::RoadNameInfo());
    segments.back().SetDistancesAndTime((i + 1) * 10, (i + 1) * 0.00001, times[i]);
  }

  using namespace routing::transit;
  Gate const gate(10, 0, true, true, 60, {1}, points[0]);
  segments[1].SetTransitInfo(std::make_unique<routing::TransitInfo>(gate, true));
  segments[2].SetTransitInfo(std::make_unique<routing::TransitInfo>(Edge(1, 2, 120, 1, false, std::vector<ShapeId>{})));
  segments[3].SetTransitInfo(std::make_unique<routing::TransitInfo>(gate, false));
  return segments;
}

df::Subroute Process(TransitRouteDisplay & display, std::vector<routing::RouteSegment> const & segments, size_t begin,
                     size_t end)
{
  df::Subroute subroute;
  subroute.m_polyline.Add(begin == 0 ? m2::PointD{} : segments[begin - 1].GetJunction().GetPoint());
  subroute.m_baseDistance = begin == 0 ? 0 : segments[begin - 1].GetDistFromBeginningMerc();
  TEST(display.ProcessSubroute({segments.begin() + begin, segments.begin() + end}, subroute), (begin, end));
  return subroute;
}

void TestPoint(m2::PointD const & actual, m2::PointD const & expected)
{
  // Transit points are quantized in the map section.
  TEST_LESS(actual.SquaredLength(expected), 1e-12, (actual, expected));
}
}  // namespace

UNIT_TEST(TransitDisplay_GateGeometrySurvivesWaypoints)
{
  Fixture fixture;
  auto const segments = MakeSegments();
  auto wholeDisplay = fixture.MakeDisplay();
  auto const whole = Process(wholeDisplay, segments, 0, segments.size());
  TEST_EQUAL(whole.m_polyline.GetSize(), 6, ());
  TEST_EQUAL(whole.m_markers.size(), 2, ());

  auto splitDisplay = fixture.MakeDisplay();
  for (size_t i = 0; i < segments.size(); ++i)
  {
    auto const subroute = Process(splitDisplay, segments, i, i + 1);
    TEST_EQUAL(subroute.m_polyline.GetSize(), 2, (i));
    TestPoint(subroute.m_polyline.Back(), segments[i].GetJunction().GetPoint());
    if (i == 1 || i == 3)
    {
      TEST_EQUAL(subroute.m_style.size(), 1, ());
      TEST_EQUAL(subroute.m_style.front().m_color, df::kRoutePedestrian, ());
      TEST(subroute.m_style.front().m_pattern.m_isDashed, ());
    }
    if (i == 2)
    {
      TEST_EQUAL(subroute.m_markers.size(), 2, ());
      TestPoint(subroute.m_markers.back().m_position, segments[i].GetJunction().GetPoint());
    }
  }
}

UNIT_TEST(TransitDisplay_GateLabelsFollowTraversal)
{
  Fixture fixture;
  auto const segments = MakeSegments();
  auto display = fixture.MakeDisplay();
  Process(display, segments, 0, 3);
  Process(display, segments, 3, segments.size());
  std::vector<TransitMarkInfo> gates;
  for (auto const & mark : display.m_transitMarks)
    if (mark.m_type == TransitMarkInfo::Type::Gate)
      gates.push_back(mark);
  TEST_EQUAL(gates.size(), 2, ());
  TEST_EQUAL(gates[0].m_point, segments[0].GetJunction().GetPoint(), ());
  TEST_EQUAL(gates[1].m_point, segments[3].GetJunction().GetPoint(), ());
  TEST_EQUAL(gates[0].m_titles.front().m_text, "Entrance", ());
  TEST_EQUAL(gates[1].m_titles.front().m_text, "Exit", ());
}

UNIT_TEST(TransitDisplay_EntranceIconSurvivesWaypoint)
{
  Fixture fixture;
  auto const segments = MakeSegments();
  auto display = fixture.MakeDisplay();
  Process(display, segments, 0, 2);
  Process(display, segments, 2, 3);
  auto const it = std::find_if(display.m_transitMarks.begin(), display.m_transitMarks.end(),
                               [](auto const & mark) { return mark.m_type == TransitMarkInfo::Type::KeyStop; });
  TEST(it != display.m_transitMarks.end(), ());
  TestPoint(it->m_point, segments[1].GetJunction().GetPoint());
  TEST_EQUAL(it->m_symbolName, "transit_subway", ());
}

UNIT_TEST(TransitDisplay_GroupedBoardingStopConnectsToRide)
{
  Fixture fixture(R"({
    "stops": [
      {"id": 1, "osm_id": "1", "line_ids": [1], "transfer_id": 100, "point": {"x": 0.00002, "y": 0}},
      {"id": 2, "osm_id": "2", "line_ids": [1], "point": {"x": 0.00003, "y": 0}},
      {"id": 3, "osm_id": "3", "line_ids": [1], "transfer_id": 100, "point": {"x": 0.00002, "y": 0.0002}}
    ],
    "transfers": [{"id": 100, "stop_ids": [1, 3], "point": {"x": 0.00002, "y": 0.0001}}],
    "lines": [{"id": 1, "interval": 60, "network_id": 1, "number": "1", "stop_ids": [3, 1, 2],
               "type": "subway", "color": "red"}],
    "shapes": [{"id": {"stop1_id": 2, "stop2_id": 100},
                "polyline": [{"x": 0.00003, "y": 0}, {"x": 0.00002, "y": 0.0001}]}],
    "networks": [{"id": 1, "title": "Transit test"}]
  })");
  for (bool const shaped : {false, true})
  {
    auto segments = MakeSegments();
    if (shaped)
      segments[2].SetTransitInfo(std::make_unique<routing::TransitInfo>(
          routing::transit::Edge(1, 2, 120, 1, false, std::vector<routing::transit::ShapeId>{{2, 100}})));
    for (size_t const begin : {0, 2})
    {
      auto display = fixture.MakeDisplay();
      if (begin > 0)
        Process(display, segments, 0, begin);
      auto const subroute = Process(display, segments, begin, segments.size());
      auto const ride = std::find_if(subroute.m_style.begin(), subroute.m_style.end(),
                                     [](auto const & style) { return !style.m_pattern.m_isDashed; });
      TEST(ride != subroute.m_style.end(), (shaped, begin));
      TEST(!subroute.m_markers.empty(), ());
      auto const & boardingPoint = subroute.m_markers.front().m_position;
      TestPoint(subroute.m_polyline.GetPoint(ride->m_startIndex), boardingPoint);
      TEST_LESS(subroute.m_polyline.CalcMinSquaredDistance(boardingPoint).first, 1e-12, (shaped, begin));
    }
  }
}

UNIT_TEST(TransitDisplay_WaypointDoesNotDuplicateStopMarks)
{
  Fixture fixture;
  auto segments = MakeSegments();
  segments[3].SetTransitInfo(std::make_unique<routing::TransitInfo>(
      routing::transit::Edge(2, 1, 60, 1, false, std::vector<routing::transit::ShapeId>{})));
  auto wholeDisplay = fixture.MakeDisplay();
  Process(wholeDisplay, segments, 0, segments.size());
  auto splitDisplay = fixture.MakeDisplay();
  Process(splitDisplay, segments, 0, 3);
  Process(splitDisplay, segments, 3, segments.size());
  TEST_EQUAL(splitDisplay.m_transitMarks.size(), wholeDisplay.m_transitMarks.size(), ());
  auto const boardingPoint = wholeDisplay.m_transitMarks[2].m_point;
  TEST_EQUAL(std::count_if(splitDisplay.m_transitMarks.begin(), splitDisplay.m_transitMarks.end(),
                           [&](auto const & mark) { return mark.m_point == boardingPoint; }),
             1, ());
}
}  // namespace transit_rendering_tests
