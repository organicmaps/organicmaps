#include "testing/testing.hpp"

#include "map/transit/transit_display.hpp"

#include "indexer/data_source.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/strings_bundle.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

class TransitRouteDisplayTest
{
  class RegisteredMwmInfo : public MwmInfo
  {
  public:
    RegisteredMwmInfo() { SetStatus(STATUS_REGISTERED); }
  };

public:
  using StopId = routing::transit::StopId;
  using LineId = routing::transit::LineId;
  using MarkType = TransitMarkInfo::Type;

  TransitRouteDisplayTest()
    : m_mwm(std::make_shared<RegisteredMwmInfo>())
    , m_reader(m_dataSource, [](FeatureCallback const &, std::vector<FeatureID> const &) { TEST(false, ()); },
               [](m2::RectD const &) { return std::vector<MwmSet::MwmId>{}; })
    , m_display(m_reader, [this](routing::NumMwmId) { return m_mwm; }, []() -> StringsBundle const &
  {
    static StringsBundle const strings;
    return strings;
  }, nullptr, m_symbolSizes)
  {
    m_reader.Stop();
  }

  void AddStop(StopId id, uint32_t featureId, double x,
               routing::transit::TransferId transferId = routing::transit::kInvalidTransferId)
  {
    AddStop(id, featureId, m2::PointD(x, 0.0), transferId);
  }

  void AddStop(StopId id, uint32_t featureId, m2::PointD const & point,
               routing::transit::TransferId transferId = routing::transit::kInvalidTransferId)
  {
    m_info.m_stopsSubway.emplace(id, routing::transit::Stop(id, id, featureId, transferId, {}, point, {}));
    if (featureId != kInvalidFeatureId)
    {
      auto & feature = m_info.m_features[FeatureID(m_mwm, featureId)];
      // Identical names ensure that feature identity, not the text, distinguishes stops.
      feature.m_title = "Station";
      feature.m_point = point;
    }
    if (transferId != routing::transit::kInvalidTransferId)
      m_info.m_transfersSubway.emplace(transferId, routing::transit::Transfer(transferId, point, {id}, {}));
  }

  void AddLine(LineId id, std::vector<StopId> const & stops, std::string const & type = "bus",
               std::string const & color = "#FF0000")
  {
    m_info.m_linesSubway.emplace(
        id, routing::transit::Line(id, std::to_string(id), "Test route", type, color, 1, {stops}, 600));
  }

  void Seed(m2::PointD const & point) { m_subroute.m_polyline.Add(point); }

  void Enter(StopId id)
  {
    auto const point = Point(id) - m2::PointD(0.0001, 0.0);
    m_subroute.m_polyline.Add(point);
    Gate(point);
  }

  void Edge(StopId from, StopId to, LineId lineId = 1, bool shaped = false)
  {
    std::vector<routing::transit::ShapeId> shapes;
    if (shaped)
    {
      shapes.emplace_back(from, to);
      m_info.m_shapesSubway.emplace(shapes.front(), routing::transit::Shape(shapes.front(), {Point(from), Point(to)}));
    }
    routing::transit::Edge const edge(from, to, 10, lineId, false /* transfer */, shapes);
    routing::TransitInfo const transitInfo(edge);
    auto const segment = Segment(Point(to), 100.0, 10);
    SubrouteSegmentParams ssp(transitInfo, m_info);
    ssp.m_mwmId = m_mwm;
    ssp.m_distance = 100.0;
    ssp.m_time = 10;
    auto const & stops = m_info.m_linesSubway.at(lineId).GetStopIds().front();
    m_display.AddEdgeSubwayForSubroute(segment, m_subroute, m_params, ssp, stops.front(), stops.back());
  }

  void Exit(StopId id) { Gate(Point(id) + m2::PointD(0.0001, 0.0)); }

  m2::PointD const & Point(StopId id) const { return m_info.m_stopsSubway.at(id).GetPoint(); }

  std::vector<TransitMarkInfo> Stops() const
  {
    std::vector<TransitMarkInfo> stops;
    for (auto const & mark : m_display.m_transitMarks)
      if (mark.m_type != MarkType::Gate)
        stops.push_back(mark);
    return stops;
  }

  df::Subroute const & Subroute() const { return m_subroute; }

  void CheckGeometry() const
  {
    auto const & points = m_subroute.m_polyline.GetPoints();
    auto const stops = Stops();
    TEST_EQUAL(stops.size(), m_subroute.m_markers.size(), ());
    for (size_t i = 0; i < stops.size(); ++i)
    {
      TEST_EQUAL(stops[i].m_point, m_subroute.m_markers[i].m_position, (i));
      TEST(std::find(points.begin(), points.end(), stops[i].m_point) != points.end(), (i, stops[i].m_point));
    }
    TEST(!m_subroute.m_style.empty(), ());
    size_t nextIndex = 0;
    for (auto const & style : m_subroute.m_style)
    {
      TEST_EQUAL(style.m_startIndex, nextIndex, ());
      TEST_LESS(style.m_startIndex, style.m_endIndex, ());
      TEST_LESS(style.m_endIndex, points.size(), ());
      nextIndex = style.m_endIndex;
    }
    TEST_EQUAL(nextIndex + 1, points.size(), ());
  }

  void CheckTotals(size_t edgeCount) const
  {
    double distance = 0.0;
    int time = 0;
    for (auto const & step : m_display.m_routeInfo.m_steps)
    {
      distance += step.m_distanceInMeters;
      time += step.m_timeInSec;
    }
    TEST_EQUAL(distance, edgeCount * 100.0 + 5.0, ());
    TEST_EQUAL(time, static_cast<int>(edgeCount) * 10 + 2, ());
    TEST_EQUAL(m_display.m_routeInfo.m_totalPedestrianDistInMeters, 5.0, ());
    TEST_EQUAL(m_display.m_routeInfo.m_totalPedestrianTimeInSec, 2, ());
  }

private:
  routing::RouteSegment Segment(m2::PointD const & point, double distance, int time)
  {
    m_params.m_prevDistance += distance;
    m_params.m_prevTime += time;
    routing::RouteSegment segment({}, {}, geometry::PointWithAltitude(point, geometry::kDefaultAltitudeMeters), {});
    segment.SetDistancesAndTime(m_params.m_prevDistance, 0.0, m_params.m_prevTime);
    return segment;
  }

  void Gate(m2::PointD const & point)
  {
    routing::transit::Gate const gate;
    routing::TransitInfo const transitInfo(gate);
    auto const segment = Segment(point, 5.0, 2);
    SubrouteSegmentParams ssp(transitInfo, m_info);
    ssp.m_mwmId = m_mwm;
    ssp.m_distance = 5.0;
    ssp.m_time = 2;
    m_display.AddGateSubwayForSubroute(segment, m_subroute, m_params, ssp);
  }

  MwmSet::MwmId m_mwm;
  FrozenDataSource m_dataSource;
  TransitReadManager m_reader;
  std::map<std::string, m2::PointF> m_symbolSizes = {{"transit_bus-s", {16.0f, 16.0f}}};
  TransitRouteDisplay m_display;
  TransitDisplayInfo m_info;
  SubrouteParams m_params;
  df::Subroute m_subroute;
};

namespace transit_display_tests
{
using MarkType = TransitMarkInfo::Type;

UNIT_TEST(TransitDisplay_StopPlatformPairs)
{
  for (auto const & type : {"bus", "tram", "trolleybus"})
  {
    TransitRouteDisplayTest display;
    display.AddStop(1, 10, 0.0);
    display.AddStop(2, 10, 0.00004);
    display.AddStop(3, 20, 0.01);
    display.AddStop(4, 20, 0.01004);
    display.AddStop(5, 30, 0.02);
    display.AddStop(6, 30, 0.02004);
    display.AddLine(1, {1, 2, 3, 4, 5, 6}, type);
    display.Enter(1);
    for (uint64_t i = 1; i < 6; ++i)
      display.Edge(i, i + 1);
    display.Exit(6);

    auto const stops = display.Stops();
    auto const & subroute = display.Subroute();
    TEST_EQUAL(stops.size(), 3, (type));
    TEST_EQUAL(subroute.m_markers.size(), stops.size(), (type));
    TEST(stops[0].m_type == MarkType::KeyStop, (type));
    TEST(stops[1].m_type == MarkType::Stop, (type));
    TEST(stops[2].m_type == MarkType::KeyStop, (type));
    TEST_EQUAL(stops[0].m_featureId.m_index, 10, (type));
    TEST_EQUAL(stops[1].m_featureId.m_index, 20, (type));
    TEST_EQUAL(stops[2].m_featureId.m_index, 30, (type));
    TEST(!stops[0].m_symbolName.empty(), (type));
    TEST(!stops[2].m_symbolName.empty(), (type));
    for (size_t i = 0; i < stops.size(); ++i)
    {
      TEST_EQUAL(stops[i].m_titles.size(), 1, (type));
      TEST_EQUAL(stops[i].m_point, subroute.m_markers[i].m_position, (type));
    }

    // Draw one platform point per stop, while retaining the original travel time and distance.
    TEST_EQUAL(subroute.m_polyline.GetSize(), 5, (type));
    for (uint64_t i = 1; i <= 3; ++i)
      TEST_EQUAL(subroute.m_polyline.GetPoint(i), display.Point(i * 2), (type));
    display.CheckGeometry();
    display.CheckTotals(5);
  }
}

UNIT_TEST(TransitDisplay_KeepDistinctStops)
{
  struct Case
  {
    uint32_t m_feature1, m_feature2;
    double m_x2;
    std::string m_type;
    bool m_shaped = false;
    routing::transit::TransferId m_transfer = routing::transit::kInvalidTransferId;
  };
  std::vector<Case> const cases = {
      {kInvalidFeatureId, kInvalidFeatureId, 0.00004, "bus"},
      {10, kInvalidFeatureId, 0.00004, "bus"},
      {10, 20, 0.00004, "bus"},  // Same title, different features / street sides.
      {10, 10, 0.002, "bus"},    // Shared feature, but more than 100 metres apart.
      {10, 10, 0.00004, "subway"},
      {10, 10, 0.00004, "bus", true},
      {10, 10, 0.00004, "bus", false, 100},
  };
  for (auto const & test : cases)
  {
    TransitRouteDisplayTest display;
    display.AddStop(1, test.m_feature1, 0.0, test.m_transfer);
    display.AddStop(2, test.m_feature2, test.m_x2);
    display.AddLine(1, {1, 2}, test.m_type);
    display.Enter(1);
    display.Edge(1, 2, 1, test.m_shaped);
    display.Exit(2);
    auto const stops = display.Stops();
    TEST_EQUAL(stops.size(), 2, (test.m_type, test.m_feature1, test.m_feature2, test.m_x2));
    TEST_EQUAL(display.Subroute().m_markers.size(), stops.size(), ());
    TEST(stops[0].m_type == MarkType::KeyStop, ());
    TEST(stops[1].m_type == MarkType::KeyStop, ());
    display.CheckGeometry();
    display.CheckTotals(1);
  }
}

UNIT_TEST(TransitDisplay_LoopKeepsRepeatedVisit)
{
  TransitRouteDisplayTest display;
  display.AddStop(1, 10, 0.0);
  display.AddStop(2, 10, 0.00004);
  display.AddStop(3, 20, 0.01);
  display.AddLine(1, {1, 2, 3, 1, 2});
  display.Enter(1);
  display.Edge(1, 2);
  display.Edge(2, 3);
  display.Edge(3, 1);
  display.Edge(1, 2);
  display.Exit(2);
  auto const stops = display.Stops();
  TEST_EQUAL(stops.size(), 3, ());
  TEST_EQUAL(display.Subroute().m_markers.size(), stops.size(), ());
  TEST_EQUAL(stops[0].m_featureId, stops[2].m_featureId, ());
  TEST_NOT_EQUAL(stops[0].m_featureId, stops[1].m_featureId, ());
  TEST(stops[0].m_type == MarkType::KeyStop, ());
  TEST(stops[2].m_type == MarkType::KeyStop, ());
  display.CheckGeometry();
  display.CheckTotals(4);
}

UNIT_TEST(TransitDisplay_PairsPreserveTransfers)
{
  for (int scenario = 0; scenario < 3; ++scenario)
  {
    TransitRouteDisplayTest display;
    display.AddStop(1, 10, 0.0);
    display.AddStop(2, 20, 0.01);
    display.AddStop(3, 20, 0.01004);
    display.AddStop(4, 30, 0.02);
    if (scenario == 0)
    {
      // The incoming line ends with a duplicate pair; its final marker becomes a transfer.
      display.AddLine(1, {1, 2, 3});
      display.AddLine(2, {3, 4}, "bus", "#0000FF");
    }
    else
    {
      // The outgoing line begins with the duplicate pair, optionally ending immediately there.
      display.AddLine(1, {1, 2});
      display.AddLine(2, scenario == 1 ? std::vector<uint64_t>{2, 3, 4} : std::vector<uint64_t>{2, 3}, "bus",
                      "#0000FF");
    }
    display.Enter(1);
    display.Edge(1, 2, 1);
    display.Edge(2, 3, scenario == 0 ? 1 : 2);
    if (scenario != 2)
      display.Edge(3, 4, 2);
    display.Exit(scenario == 2 ? 3 : 4);

    auto const stops = display.Stops();
    auto const & markers = display.Subroute().m_markers;
    TEST_EQUAL(stops.size(), scenario == 2 ? 2 : 3, (scenario));
    TEST_EQUAL(markers.size(), stops.size(), (scenario));
    TEST(stops[1].m_type == MarkType::Transfer, (scenario));
    TEST_EQUAL(stops[1].m_titles.size(), 2, (scenario));
    TEST_EQUAL(markers[1].m_colors.size(), 2, (scenario));
    TEST_GREATER(markers[1].m_scale, markers[0].m_scale, (scenario));
    TEST_EQUAL(stops[1].m_point, markers[1].m_position, (scenario));
    display.CheckGeometry();
    display.CheckTotals(scenario == 2 ? 2 : 3);
  }
}

UNIT_TEST(TransitDisplay_OffAxisStopChain)
{
  TransitRouteDisplayTest display;
  display.AddStop(1, 10, -0.01);
  display.AddStop(2, 20, 0.0);
  display.AddStop(3, 20, m2::PointD(0.00005, 0.0002));
  display.AddStop(4, 20, 0.0001);
  display.AddStop(5, 30, 0.01);
  display.AddLine(1, {1, 2, 3, 4, 5});
  display.Enter(1);
  for (uint64_t i = 1; i < 5; ++i)
    display.Edge(i, i + 1);
  display.Exit(5);

  auto const & line = display.Subroute().m_polyline;
  TEST_EQUAL(line.GetSize(), 5, ());
  TEST_EQUAL(line.GetPoint(1), display.Point(1), ());
  TEST_EQUAL(line.GetPoint(2), display.Point(4), ());
  TEST_EQUAL(line.GetPoint(3), display.Point(5), ());
  TEST_EQUAL(display.Stops().size(), 3, ());
  display.CheckGeometry();
  display.CheckTotals(4);
}

UNIT_TEST(TransitDisplay_PairRetainsSeededEndpoint)
{
  TransitRouteDisplayTest display;
  display.AddStop(1, 10, 0.0);
  display.AddStop(2, 10, m2::PointD(0.00004, 0.00004));
  display.AddLine(1, {1, 2});
  // A subroute starting at this sole vertex still needs the connection to the platform.
  display.Seed(display.Point(1));
  display.Edge(1, 2);
  display.Exit(2);

  auto const & line = display.Subroute().m_polyline;
  TEST_EQUAL(line.GetSize(), 3, ());
  TEST_EQUAL(line.GetPoint(0), display.Point(1), ());
  TEST_EQUAL(line.GetPoint(1), display.Point(2), ());
  TEST_EQUAL(display.Stops().size(), 1, ());
  display.CheckGeometry();
  display.CheckTotals(1);
}

UNIT_TEST(TransitDisplay_PairAfterTransferRetainsPreviousEndpoint)
{
  TransitRouteDisplayTest display;
  display.AddStop(1, 10, 0.0);
  display.AddStop(2, 20, 0.01);
  display.AddStop(3, 30, m2::PointD(0.0101, 0.0001));
  display.AddStop(4, 30, m2::PointD(0.01015, 0.00012));
  display.AddStop(5, 40, 0.02);
  display.AddLine(1, {1, 2});
  display.AddLine(2, {3, 4, 5}, "bus", "#0000FF");
  display.Enter(1);
  display.Edge(1, 2, 1);
  // Transfer segments leave the display tail at stop 2; stop 3 is not that endpoint.
  display.Edge(3, 4, 2);
  display.Edge(4, 5, 2);
  display.Exit(5);

  auto const & line = display.Subroute().m_polyline;
  TEST_EQUAL(line.GetSize(), 6, ());
  TEST_EQUAL(line.GetPoint(1), display.Point(1), ());
  TEST_EQUAL(line.GetPoint(2), display.Point(2), ());
  TEST_EQUAL(line.GetPoint(3), display.Point(4), ());
  TEST_EQUAL(line.GetPoint(4), display.Point(5), ());
  TEST_EQUAL(display.Stops().size(), 3, ());
  display.CheckGeometry();
  display.CheckTotals(3);
}
}  // namespace transit_display_tests
