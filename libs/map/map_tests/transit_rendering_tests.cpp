#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "map/transit/transit_display.hpp"

#include "drape_frontend/route_renderer.hpp"

#include "routing/fake_feature_ids.hpp"

#include "transit/transit_graph_data.hpp"

#include "coding/files_container.hpp"

#include "base/strings_bundle.hpp"

#include <algorithm>
#include <cmath>

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

  TransitRouteDisplay MakeDisplay(BookmarkManager * bookmarks = nullptr)
  {
    return TransitRouteDisplay(m_reader, [this](routing::NumMwmId) { return m_id; },
                               [this]() -> StringsBundle const & { return m_strings; }, bookmarks, m_symbols);
  }

  void UnregisterMap() { TEST(m_dataSource.DeregisterMap(m_files.back().GetCountryFile()), ()); }

  void RestoreMap()
  {
    auto const result = m_dataSource.RegisterMap(m_files.back());
    TEST_EQUAL(result.second, MwmSet::RegResult::Success, ());
    m_id = result.first;
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

void TestTransferMarkerAxis(routing::transit::StopId firstStop)
{
  Fixture fixture(R"({
    "stops": [
      {"id": 1, "osm_id": "1", "line_ids": [1], "point": {"x": 0, "y": 0}},
      {"id": 2, "osm_id": "2", "line_ids": [1], "point": {"x": 0.00001, "y": 0}},
      {"id": 3, "osm_id": "3", "line_ids": [1, 2], "point": {"x": 0.00002, "y": 0}},
      {"id": 4, "osm_id": "4", "line_ids": [1, 2], "point": {"x": 0.00003, "y": 0}},
      {"id": 5, "osm_id": "5", "line_ids": [1], "point": {"x": 0.00003, "y": 0}},
      {"id": 6, "osm_id": "6", "line_ids": [1, 2], "point": {"x": 0.00003, "y": 0}},
      {"id": 7, "osm_id": "7", "line_ids": [2], "point": {"x": 0.00003, "y": 0}}
    ],
    "lines": [
      {"id": 1, "interval": 60, "network_id": 1, "number": "1", "stop_ids": [1, 2, 3, 4, 5, 6],
       "type": "bus", "color": "red"},
      {"id": 2, "interval": 60, "network_id": 1, "number": "2", "stop_ids": [3, 4, 6, 7],
       "type": "bus", "color": "blue"}
    ],
    "networks": [{"id": 1, "title": "Transit test"}]
  })");
  bool const coincident = firstStop == 4;
  std::vector<routing::RouteSegment> segments;
  for (size_t i = 0; i < 3; ++i)
  {
    m2::PointD const point(coincident ? 0.00003 : (i + 1) * 0.00001, 0);
    segments.emplace_back(routing::Segment(0, routing::FakeFeatureIds::kTransitGraphFeaturesStart + i, 0, true),
                          routing::turns::TurnItem(), geometry::PointWithAltitude(point, 0),
                          routing::RouteSegment::RoadNameInfo());
    segments.back().SetDistancesAndTime(coincident ? 0 : (i + 1) * 10, coincident ? 0 : point.x,
                                        (i + 1) * 120 + (i < 2 ? 30 : 60));
    segments.back().SetTransitInfo(std::make_unique<routing::TransitInfo>(routing::transit::Edge(
        firstStop + i, firstStop + i + 1, 120, i < 2 ? 1 : 2, false, std::vector<routing::transit::ShapeId>{})));
  }
  auto display = fixture.MakeDisplay();
  df::Subroute subroute;
  subroute.m_polyline.Add(m2::PointD(coincident ? 0.00003 : 0, 0));
  TEST(display.ProcessSubroute(segments, subroute), ());
  TEST_EQUAL(subroute.m_markers.size(), 4, ());
  auto const & transfer = subroute.m_markers[2];
  TEST_EQUAL(transfer.m_colors.size(), 2, ());
  TEST_EQUAL(transfer.m_colors.front(), subroute.m_markers[1].m_colors.front(), ());
  TEST_EQUAL(transfer.m_colors.back(), subroute.m_markers[3].m_colors.front(), ());
  TEST_NOT_EQUAL(transfer.m_colors.front(), transfer.m_colors.back(), ());
  TEST_ALMOST_EQUAL_ABS(transfer.m_up.Length(), 1.0, 1e-12, (transfer.m_up));
  auto const & points = subroute.m_polyline.GetPoints();
  if (coincident)
  {
    TEST_EQUAL(points[1], points[2], ());
    TEST_EQUAL(points[2], points[3], ());
  }
  else
  {
    TEST_ALMOST_EQUAL_ABS(m2::DotProduct(transfer.m_up, (points[3] - points[2]).Normalize()), 0.0, 1e-12, ());
  }
}

void TestUnavailableSummary(bool hasPrefix)
{
  Fixture fixture;
  StringsBundle const strings;
  BookmarkManager bookmarks(BookmarkManager::Callbacks([&strings]() -> StringsBundle const & { return strings; },
                                                       nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
  auto display = fixture.MakeDisplay(&bookmarks);
  auto const segments = MakeSegments();
  auto makeSubroute = [&](size_t begin)
  {
    df::Subroute subroute;
    subroute.m_polyline.Add(begin == 0 ? m2::PointD{} : segments[begin - 1].GetJunction().GetPoint());
    subroute.m_baseDistance = begin == 0 ? 0 : segments[begin - 1].GetDistFromBeginningMerc();
    return subroute;
  };
  if (hasPrefix)
  {
    Process(display, segments, 0, 3);
    TEST(!display.GetRouteInfo().m_steps.empty(), ());
  }
  fixture.UnregisterMap();
  size_t const begin = hasPrefix ? 3 : 0;
  auto subroute = makeSubroute(begin);
  TEST(!display.ProcessSubroute({segments.begin() + begin, segments.end()}, subroute), (hasPrefix));
  TEST(display.GetRouteInfo().m_steps.empty(), (hasPrefix));

  routing::RouteSegment walk(routing::Segment(0, 0, 5, true), routing::turns::TurnItem(),
                             geometry::PointWithAltitude({0.00006, 0.00002}, 0), routing::RouteSegment::RoadNameInfo());
  walk.SetDistancesAndTime(60, 0.00006, 270);
  subroute = makeSubroute(segments.size());
  TEST(!display.ProcessSubroute({walk}, subroute), (hasPrefix));
  auto const & info = display.GetRouteInfo();
  TEST(info.m_steps.empty(), (hasPrefix));
  TEST_EQUAL(info.m_totalTimeInSec, 0, ());
  TEST_EQUAL(info.m_totalDistInMeters, 0, ());
  TEST_EQUAL(info.m_totalPedestrianTimeInSec, 0, ());
  TEST_EQUAL(info.m_totalPedestrianDistInMeters, 0, ());
  TEST(info.m_totalDistanceStr.empty(), ());
  display.CreateTransitMarks();
  TEST(bookmarks.GetUserMarkIds(UserMark::Type::TRANSIT).empty(), (hasPrefix));

  fixture.RestoreMap();
  subroute = makeSubroute(0);
  TEST(!display.ProcessSubroute(segments, subroute), (hasPrefix));
  auto rebuiltDisplay = fixture.MakeDisplay();
  TEST(rebuiltDisplay.ProcessSubroute(segments, subroute), (hasPrefix));
  TEST(!rebuiltDisplay.GetRouteInfo().m_steps.empty(), ());
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

UNIT_TEST(TransitDisplay_ExitIconSurvivesWaypoint)
{
  Fixture fixture;
  auto const segments = MakeSegments();
  auto wholeDisplay = fixture.MakeDisplay();
  auto const whole = Process(wholeDisplay, segments, 0, segments.size());
  auto const countKeyStops = [](auto const & marks)
  {
    return std::count_if(marks.begin(), marks.end(),
                         [](auto const & mark) { return mark.m_type == TransitMarkInfo::Type::KeyStop; });
  };
  TEST_EQUAL(countKeyStops(wholeDisplay.m_transitMarks), 2, ());
  for (std::vector<size_t> const & ends : {std::vector<size_t>{3, 5}, {1, 2, 3, 4, 5}})
  {
    auto splitDisplay = fixture.MakeDisplay();
    size_t markerCount = 0;
    size_t begin = 0;
    for (size_t const end : ends)
    {
      markerCount += Process(splitDisplay, segments, begin, end).m_markers.size();
      begin = end;
    }
    TEST_EQUAL(countKeyStops(splitDisplay.m_transitMarks), 2, (ends));
    TEST_EQUAL(markerCount, whole.m_markers.size(), (ends));
    TEST_EQUAL(splitDisplay.m_transitMarks.size(), wholeDisplay.m_transitMarks.size(), (ends));
    for (size_t i = 0; i < wholeDisplay.m_transitMarks.size(); ++i)
    {
      auto const & actual = splitDisplay.m_transitMarks[i];
      auto const & expected = wholeDisplay.m_transitMarks[i];
      TEST(actual.m_type == expected.m_type, (ends, i));
      TEST_EQUAL(actual.m_point, expected.m_point, (ends, i));
      TEST_EQUAL(actual.m_symbolName, expected.m_symbolName, (ends, i));
      TEST_EQUAL(actual.m_color, expected.m_color, (ends, i));
    }
  }
}

UNIT_TEST(TransitDisplay_WalkClearsExitIconContext)
{
  Fixture fixture;
  auto segments = MakeSegments();
  segments[3].SetTransitInfo({});
  segments[4].SetTransitInfo(std::make_unique<routing::TransitInfo>(routing::transit::Gate(), false));
  for (bool const splitWalk : {false, true})
  {
    auto display = fixture.MakeDisplay();
    Process(display, segments, 0, 3);
    if (splitWalk)
    {
      Process(display, segments, 3, 4);
      Process(display, segments, 4, 5);
    }
    else
    {
      Process(display, segments, 3, 5);
    }
    TEST_EQUAL(display.m_transitMarks.size(), 4, (splitWalk));
    TEST_EQUAL(std::count_if(display.m_transitMarks.begin(), display.m_transitMarks.end(),
                             [](auto const & mark) { return mark.m_type == TransitMarkInfo::Type::KeyStop; }),
               1, (splitWalk));
    auto const & alighting = display.m_transitMarks[2];
    TEST(alighting.m_type == TransitMarkInfo::Type::Stop, (splitWalk));
    TestPoint(alighting.m_point, segments[2].GetJunction().GetPoint());
  }
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
  auto splitDisplay = fixture.MakeDisplay();
  Process(splitDisplay, segments, 0, 3);
  Process(splitDisplay, segments, 3, segments.size());
  TEST_EQUAL(splitDisplay.m_transitMarks.size(), wholeDisplay.m_transitMarks.size(), ());
  auto const boardingPoint = wholeDisplay.m_transitMarks[2].m_point;
  TEST_EQUAL(std::count_if(splitDisplay.m_transitMarks.begin(), splitDisplay.m_transitMarks.end(),
                           [&](auto const & mark) { return mark.m_point == boardingPoint; }),
             1, ());
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
      TEST_EQUAL(step.m_colorARGB, info.m_steps.front().m_colorARGB, ());
      TEST_EQUAL(step.m_timeInSec, changesLine ? 150 : 270, ());
      TEST_EQUAL(step.m_distanceInMeters, changesLine ? 10 : 20, ());
    }
  }
}

UNIT_TEST(TransitDisplay_StraightTransferKeepsColoredRing)
{
  TestTransferMarkerAxis(1);
}

UNIT_TEST(TransitDisplay_CoincidentStopsKeepColoredRing)
{
  TestTransferMarkerAxis(4);
}

UNIT_TEST(TransitDisplay_DirectionRoundingKeepsTransferAxis)
{
  Fixture fixture;
  auto const segments = MakeSegments();
  auto const & segment = segments[2];
  auto display = fixture.MakeDisplay();
  TransitDisplayInfos infos;
  display.CollectTransitDisplayInfo({segment}, infos);
  TEST(display.m_transitReadManager.GetTransitDisplayInfo(infos), ());
  auto const & info = *infos.begin()->second;
  auto const & start = info.m_stopsSubway.at(1).GetPoint();
  auto const & finish = info.m_stopsSubway.at(2).GetPoint();
  auto const direction = (finish - start).Normalize();
  for (bool const almostStraight : {false, true})
  {
    df::Subroute subroute;
    subroute.m_polyline.Add(start);
    SubrouteParams sp;
    sp.m_lastLineId = 2;
    sp.m_lastColor = df::GetTransitColorName("green");
    // Normalizing collinear legs of different lengths can change a component by one ULP.
    sp.m_lastDir = almostStraight ? m2::PointD(std::nextafter(direction.x, 0.0), direction.y) : m2::PointD(0, 1);
    sp.m_marker.m_colors.push_back(sp.m_lastColor);
    SubrouteSegmentParams ssp(segment.GetTransitInfo(), info);
    ssp.m_mwmId = infos.begin()->first;
    display.AddEdgeSubwayForSubroute(segment, subroute, sp, ssp, 1, 2);
    TEST_EQUAL(subroute.m_markers.size(), 1, ());
    auto const & marker = subroute.m_markers.front();
    TEST_EQUAL(marker.m_colors,
               (std::vector<std::string>{df::GetTransitColorName("green"), df::GetTransitColorName("red")}), ());
    TEST_ALMOST_EQUAL_ABS(marker.m_up.Length(), 1.0, 1e-12, (almostStraight));
    if (almostStraight)
      TEST_ALMOST_EQUAL_ABS(m2::DotProduct(marker.m_up, direction), 0.0, 1e-12, (marker.m_up));
    else
    {
      TEST_ALMOST_EQUAL_ABS(marker.m_up.x, -std::sqrt(0.5), 1e-12, ());
      TEST_ALMOST_EQUAL_ABS(marker.m_up.y, std::sqrt(0.5), 1e-12, ());
    }
  }
}

UNIT_TEST(TransitDisplay_WalkSeparatesRideStopsAndColors)
{
  Fixture fixture(R"({
    "stops": [
      {"id": 1, "osm_id": "1", "line_ids": [1], "point": {"x": 0.00001, "y": 0}},
      {"id": 2, "osm_id": "2", "line_ids": [1], "point": {"x": 0.00002, "y": 0}},
      {"id": 3, "osm_id": "3", "line_ids": [2], "point": {"x": 0.00002, "y": 0}},
      {"id": 4, "osm_id": "4", "line_ids": [2], "point": {"x": 0.00004, "y": 0}}
    ],
    "lines": [
      {"id": 1, "interval": 60, "network_id": 1, "number": "1", "stop_ids": [1, 2],
       "type": "bus", "color": "red"},
      {"id": 2, "interval": 60, "network_id": 1, "number": "2", "stop_ids": [3, 4],
       "type": "bus", "color": "blue"}
    ],
    "networks": [{"id": 1, "title": "Transit test"}]
  })");
  // Walking away and back separates the co-located alighting and boarding stops.
  std::vector<m2::PointD> const points = {{0.00001, 0}, {0.00002, 0}, {0.00003, 0},
                                          {0.00002, 0}, {0.00004, 0}, {0.00005, 0}};
  std::vector<double> const distances = {0.00001, 0.00002, 0.00003, 0.00004, 0.00006, 0.00007};
  std::vector<double> const times = {10, 160, 170, 180, 330, 340};
  std::vector<routing::RouteSegment> segments;
  for (size_t i = 0; i < points.size(); ++i)
  {
    segments.emplace_back(routing::Segment(0, routing::FakeFeatureIds::kTransitGraphFeaturesStart + i, 0, true),
                          routing::turns::TurnItem(), geometry::PointWithAltitude(points[i], 0),
                          routing::RouteSegment::RoadNameInfo());
    segments.back().SetDistancesAndTime((i + 1) * 10, distances[i], times[i]);
  }
  for (size_t const index : {1, 4})
    segments[index].SetTransitInfo(std::make_unique<routing::TransitInfo>(
        routing::transit::Edge(index == 1 ? 1 : 3, index == 1 ? 2 : 4, 120, index == 1 ? 1 : 2, false,
                               std::vector<routing::transit::ShapeId>{})));

  for (std::vector<size_t> const & ends : {std::vector<size_t>{6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {1, 2, 3, 4, 5, 6}})
  {
    auto display = fixture.MakeDisplay();
    std::vector<df::SubrouteMarker> markers;
    size_t begin = 0;
    for (size_t const end : ends)
    {
      auto const subroute = Process(display, segments, begin, end);
      markers.insert(markers.end(), subroute.m_markers.begin(), subroute.m_markers.end());
      begin = end;
    }
    TEST_EQUAL(markers.size(), 4, (ends));
    TEST_EQUAL(display.m_transitMarks.size(), 4, (ends));
    size_t const stopIndices[] = {0, 1, 3, 4};
    for (size_t i = 0; i < markers.size(); ++i)
    {
      TestPoint(markers[i].m_position, points[stopIndices[i]]);
      TestPoint(display.m_transitMarks[i].m_point, points[stopIndices[i]]);
      TEST_EQUAL(markers[i].m_colors.size(), 1, (ends, i));
      TEST_EQUAL(markers[i].m_colors.front(), df::GetTransitColorName(i < 2 ? "red" : "blue"), (ends, i));
    }
    auto const & info = display.GetRouteInfo();
    TEST_EQUAL(info.m_totalTimeInSec, 340, (ends));
    TEST_EQUAL(info.m_totalPedestrianTimeInSec, 40, (ends));
    TEST_EQUAL(info.m_totalDistInMeters, 60, (ends));
    TEST_EQUAL(info.m_totalPedestrianDistInMeters, 40, (ends));
  }
}

UNIT_TEST(TransitDisplay_UnavailableFirstLegStaysUnavailable)
{
  TestUnavailableSummary(false);
}

UNIT_TEST(TransitDisplay_UnavailableLegDiscardsPrefix)
{
  TestUnavailableSummary(true);
}

UNIT_TEST(TransitDisplay_UnavailableLegClearsPendingStop)
{
  Fixture fixture;
  auto display = fixture.MakeDisplay();
  auto const segments = MakeSegments();
  Process(display, segments, 0, 3);
  TEST(display.m_pendingStopIndex, ());
  fixture.UnregisterMap();
  df::Subroute subroute;
  subroute.m_polyline.Add(segments[2].GetJunction().GetPoint());
  subroute.m_baseDistance = segments[2].GetDistFromBeginningMerc();
  TEST(!display.ProcessSubroute({segments[3]}, subroute), ());
  TEST(!display.m_pendingStopIndex, ());
}

UNIT_TEST(TransitDisplay_WalkingOnlyZeroTimeSummaryIsAvailable)
{
  Fixture fixture;
  auto display = fixture.MakeDisplay();
  auto walk = MakeSegments().front();
  walk.SetDistancesAndTime(10, 0.00001, 0);
  Process(display, {walk}, 0, 1);
  auto const & info = display.GetRouteInfo();
  TEST_EQUAL(info.m_steps.size(), 1, ());
  TEST_EQUAL(info.m_steps.front().m_type, TransitType::Pedestrian, ());
  TEST_EQUAL(info.m_totalTimeInSec, 0, ());
}
}  // namespace transit_rendering_tests
