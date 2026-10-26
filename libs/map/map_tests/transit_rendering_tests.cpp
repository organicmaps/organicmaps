#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "map/transit/transit_display.hpp"

#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "routing/fake_feature_ids.hpp"

#include "transit/transit_graph_data.hpp"

#include "coding/files_container.hpp"

#include "base/strings_bundle.hpp"

#include <algorithm>

namespace transit_rendering_tests
{
namespace
{
// A named bus stop feature, which names the transit stop at the same point.
struct StopPoi
{
  m2::PointD m_point;
  std::string m_name;
};

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
      })",
                   std::vector<StopPoi> const & stopPois = {})
    : m_reader(m_dataSource, [this](FeatureCallback const & fn, std::vector<FeatureID> const & ids)
  { m_dataSource.ReadFeatures(fn, ids); }, [](m2::RectD const &) { return std::vector<MwmSet::MwmId>(); })
    , m_bookmarks(BookmarkManager::Callbacks([this]() -> StringsBundle const & { return m_strings; }, nullptr, nullptr,
                                             nullptr, nullptr, nullptr, nullptr))
  {
    df::VisualParams::Init(1.0, 256);
    m_strings.SetString("core_entrance", "Entrance");
    m_strings.SetString("core_exit", "Exit");
    m_id =
        BuildCountry("TransitRenderingTest", [&graphJson, &stopPois](generator::tests_support::TestMwmBuilder & builder)
    {
      generator::tests_support::TestPOI poi({0.00001, 0}, "Gate", "en");
      poi.SetTypes({{"railway", "subway_entrance"}});
      builder.Add(poi);
      for (auto const & stop : stopPois)
      {
        generator::tests_support::TestPOI stopPoi(stop.m_point, stop.m_name, "en");
        stopPoi.SetTypes({{"highway", "bus_stop"}});
        builder.Add(stopPoi);
      }
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
                               [this]() -> StringsBundle const & { return m_strings; }, &m_bookmarks, m_symbols);
  }

  std::vector<TransitMark const *> GetMarks(TransitRouteDisplay & display)
  {
    m_bookmarks.GetEditSession().ClearGroup(UserMark::Type::TRANSIT);
    display.CreateTransitMarks();
    std::vector<TransitMark const *> marks;
    for (auto const id : m_bookmarks.GetUserMarkIds(UserMark::Type::TRANSIT))
      marks.push_back(m_bookmarks.GetMark<TransitMark>(id));
    std::sort(marks.begin(), marks.end(), [](auto const * a, auto const * b) { return a->GetIndex() < b->GetIndex(); });
    return marks;
  }

private:
  MwmSet::MwmId m_id;
  TransitReadManager m_reader;
  StringsBundle m_strings;
  BookmarkManager m_bookmarks;
  std::map<std::string, m2::PointF> const m_symbols = {{"transit_subway-s", {8, 8}}, {"transit_subway-m", {12, 12}}};
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
  std::vector<TransitMark const *> gates;
  for (auto const * mark : fixture.GetMarks(display))
    if (mark->GetPriority() == static_cast<uint16_t>(UserMark::Priority::TransitGate))
      gates.push_back(mark);
  TEST_EQUAL(gates.size(), 2, ());
  TEST_EQUAL(gates[0]->GetPivot(), segments[0].GetJunction().GetPoint(), ());
  TEST_EQUAL(gates[1]->GetPivot(), segments[3].GetJunction().GetPoint(), ());
  TEST_EQUAL(gates[0]->GetTitleDecl()->front().m_primaryText, "Entrance", ());
  TEST_EQUAL(gates[1]->GetTitleDecl()->front().m_primaryText, "Exit", ());
}

UNIT_TEST(TransitDisplay_EntranceIconSurvivesWaypoint)
{
  Fixture fixture;
  auto const segments = MakeSegments();
  auto display = fixture.MakeDisplay();
  Process(display, segments, 0, 2);
  Process(display, segments, 2, 3);
  auto const marks = fixture.GetMarks(display);
  auto const it = std::find_if(marks.begin(), marks.end(), [](auto const * mark)
  { return mark->GetPriority() == static_cast<uint16_t>(UserMark::Priority::TransitKeyStop); });
  TEST(it != marks.end(), ());
  TestPoint((*it)->GetPivot(), segments[1].GetJunction().GetPoint());
  TEST_EQUAL((*it)->GetSymbolNames()->begin()->second, "transit_subway-s", ());
}

UNIT_TEST(TransitDisplay_MarkersUseSubrouteMercatorDistances)
{
  Fixture fixture;
  auto const segments = MakeSegments();
  for (size_t begin : {0, 1, 2})
  {
    auto display = fixture.MakeDisplay();
    if (begin > 0)
      Process(display, segments, 0, begin);
    auto const subroute = Process(display, segments, begin, segments.size());
    TEST_EQUAL(subroute.m_markers.size(), 2, (begin));
    TEST_ALMOST_EQUAL_ABS(subroute.m_markers.front().m_distance,
                          segments[1].GetDistFromBeginningMerc() - subroute.m_baseDistance, 1e-12, (begin));
    TEST_ALMOST_EQUAL_ABS(subroute.m_markers.back().m_distance,
                          segments[2].GetDistFromBeginningMerc() - subroute.m_baseDistance, 1e-12, (begin));
  }
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
  auto const wholeMarks = fixture.GetMarks(wholeDisplay);
  auto const boardingPoint = wholeMarks[2]->GetPivot();
  auto splitDisplay = fixture.MakeDisplay();
  Process(splitDisplay, segments, 0, 3);
  Process(splitDisplay, segments, 3, segments.size());
  auto const splitMarks = fixture.GetMarks(splitDisplay);
  TEST_EQUAL(splitMarks.size(), wholeMarks.size(), ());
  TEST_EQUAL(std::count_if(splitMarks.begin(), splitMarks.end(),
                           [&](auto const * mark) { return mark->GetPivot() == boardingPoint; }),
             1, ());
}

UNIT_TEST(TransitDisplay_GateSeparatesStopVisitsAcrossWaypoints)
{
  Fixture fixture;
  auto segments = MakeSegments();
  segments[4].SetTransitInfo(std::make_unique<routing::TransitInfo>(
      routing::transit::Edge(2, 1, 10, 1, false, std::vector<routing::transit::ShapeId>{})));
  auto display = fixture.MakeDisplay();
  Process(display, segments, 0, 3);
  auto const alightingPoint = fixture.GetMarks(display).back()->GetPivot();
  // The gate is the only segment between the two waypoints.
  Process(display, segments, 3, 4);
  Process(display, segments, 4, segments.size());
  auto const marks = fixture.GetMarks(display);
  TEST_EQUAL(
      std::count_if(marks.begin(), marks.end(), [&](auto const * mark) { return mark->GetPivot() == alightingPoint; }),
      2, ());
}

UNIT_TEST(TransitDisplay_LineIdentitySeparatesMatchingBadges)
{
  Fixture fixture(R"({
    "stops": [
      {"id": 1, "osm_id": "1", "line_ids": [1], "point": {"x": 0, "y": 0}},
      {"id": 2, "osm_id": "2", "line_ids": [1, 2], "point": {"x": 0.00001, "y": 0}},
      {"id": 3, "osm_id": "3", "line_ids": [1, 2], "point": {"x": 0.00002, "y": 0}}
    ],
    "lines": [
      {"id": 1, "interval": 60, "network_id": 1, "number": "7", "stop_ids": [1, 2, 3],
       "type": "bus", "color": "red"},
      {"id": 2, "interval": 60, "network_id": 1, "number": "7", "stop_ids": [2, 3],
       "type": "bus", "color": "red"}
    ],
    "networks": [{"id": 1, "title": "Transit test"}]
  })");
  for (routing::transit::LineId const secondLine : {1, 2})
  {
    bool const changesLine = secondLine != 1;
    std::vector<routing::RouteSegment> segments;
    for (size_t i = 0; i < 2; ++i)
    {
      m2::PointD const point((i + 1) * 0.00001, 0);
      segments.emplace_back(routing::Segment(0, routing::FakeFeatureIds::kTransitGraphFeaturesStart + i, 0, true),
                            routing::turns::TurnItem(), geometry::PointWithAltitude(point, 0),
                            routing::RouteSegment::RoadNameInfo());
      segments.back().SetDistancesAndTime((i + 1) * 10, point.x, i == 0 ? 150 : (changesLine ? 300 : 270));
      segments.back().SetTransitInfo(std::make_unique<routing::TransitInfo>(routing::transit::Edge(
          i + 1, i + 2, 120, i == 0 ? 1 : secondLine, false, std::vector<routing::transit::ShapeId>{})));
    }
    auto display = fixture.MakeDisplay();
    Process(display, segments, 0, segments.size());
    auto const & info = display.GetRouteInfo();
    TEST_EQUAL(info.m_steps.size(), changesLine ? 2 : 1, (secondLine));
    TEST_EQUAL(info.m_totalDistInMeters, 20, ());
    TEST_EQUAL(info.m_totalTimeInSec, changesLine ? 300 : 270, ());
    for (auto const & step : info.m_steps)
    {
      TEST_EQUAL(step.m_type, TransitType::Bus, ());
      TEST_EQUAL(step.m_number, "7", ());
      TEST_EQUAL(step.m_lineNumber, "7", ());
      TEST_EQUAL(step.m_colorARGB, info.m_steps.front().m_colorARGB, ());
      TEST_EQUAL(step.m_timeInSec, changesLine ? 150 : 270, ());
      TEST_EQUAL(step.m_distanceInMeters, changesLine ? 10 : 20, ());
      TEST_EQUAL(step.m_stopCount, changesLine ? 1 : 2, ());
    }
  }
}

UNIT_TEST(TransitDisplay_ParallelLinesKeepSelectedLineStops)
{
  // The route rides line 1 through A-B-C-D; line 2 also serves A and D, but via X.
  std::vector<m2::PointD> const points = {{0.001, 0.001}, {0.002, 0.001}, {0.003, 0.001}, {0.004, 0.001}};
  Fixture fixture(R"({
    "stops": [
      {"id": 1, "osm_id": "1", "line_ids": [1, 2], "point": {"x": 0.001, "y": 0.001}},
      {"id": 2, "osm_id": "2", "line_ids": [1], "point": {"x": 0.002, "y": 0.001}},
      {"id": 3, "osm_id": "3", "line_ids": [1], "point": {"x": 0.003, "y": 0.001}},
      {"id": 4, "osm_id": "4", "line_ids": [1, 2], "point": {"x": 0.004, "y": 0.001}},
      {"id": 5, "osm_id": "5", "line_ids": [2], "point": {"x": 0.0025, "y": 0.003}}
    ],
    "lines": [
      {"id": 1, "interval": 60, "network_id": 1, "number": "1", "stop_ids": [1, 2, 3, 4],
       "type": "bus", "color": "red"},
      {"id": 2, "interval": 60, "network_id": 1, "number": "2", "stop_ids": [1, 5, 4],
       "type": "bus", "color": "red"}
    ],
    "networks": [{"id": 1, "title": "Transit test"}]
  })",
                  {{points[0], "A"}, {points[1], "B"}, {points[2], "C"}, {points[3], "D"}, {{0.0025, 0.003}, "X"}});

  std::vector<routing::RouteSegment> segments;
  for (size_t i = 0; i + 1 < points.size(); ++i)
  {
    segments.emplace_back(routing::Segment(0, routing::FakeFeatureIds::kTransitGraphFeaturesStart + i, 0, true),
                          routing::turns::TurnItem(), geometry::PointWithAltitude(points[i + 1], 0),
                          routing::RouteSegment::RoadNameInfo());
    segments.back().SetDistancesAndTime((i + 1) * 100, points[i + 1].x, (i + 1) * 60);
    segments.back().SetTransitInfo(std::make_unique<routing::TransitInfo>(
        routing::transit::Edge(i + 1, i + 2, 60, 1, false, std::vector<routing::transit::ShapeId>{})));
  }
  auto display = fixture.MakeDisplay();
  Process(display, segments, 0, segments.size());

  auto const & steps = display.GetRouteInfo().m_steps;
  TEST_EQUAL(steps.size(), 1, ());
  auto const & ride = steps.front();
  TEST_EQUAL(ride.m_number, "1, 2", ());
  TEST_EQUAL(ride.m_lineNumber, "1", ());
  TEST_EQUAL(ride.m_startStopName, "A", ());
  TEST_EQUAL(ride.m_endStopName, "D", ());
  TEST_EQUAL(ride.m_intermediateStopNames, std::vector<std::string>({"B", "C"}), ());
  TEST_EQUAL(ride.m_stopCount, 3, ());
}
}  // namespace transit_rendering_tests
