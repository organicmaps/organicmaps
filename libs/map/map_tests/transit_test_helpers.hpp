#pragma once

#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "map/transit/transit_display.hpp"

#include "transit/transit_graph_data.hpp"

#include "coding/files_container.hpp"

#include "base/strings_bundle.hpp"

#include <numeric>

namespace transit_tests
{
inline routing::RouteSegment MakeSegment(size_t index, double time)
{
  routing::RouteSegment segment(routing::Segment(0, 0, index, true), routing::turns::TurnItem(),
                                geometry::PointWithAltitude({(index + 1) * 0.00001, 0}, 0),
                                routing::RouteSegment::RoadNameInfo());
  segment.SetDistancesAndTime((index + 1) * 10, (index + 1) * 0.00001, time);
  return segment;
}

class Fixture : public generator::tests_support::TestWithCustomMwms
{
public:
  Fixture()
    : m_reader(m_dataSource, [](FeatureCallback const &, std::vector<FeatureID> const &) {},
               [](m2::RectD const &) { return std::vector<MwmSet::MwmId>(); })
  {
    m_id = BuildCountry("TransitTest", [](generator::tests_support::TestMwmBuilder & builder)
    {
      generator::tests_support::TestPOI poi({0, 0}, "Cafe", "en");
      poi.SetTypes({{"amenity", "cafe"}});
      builder.Add(poi);
      builder.Finish();

      routing::transit::GraphData graph;
      graph.DeserializeFromJson(R"({
        "stops": [
          {"id": 1, "osm_id": "1", "line_ids": [1], "point": {"x": 0.00002, "y": 0}},
          {"id": 2, "osm_id": "2", "line_ids": [1], "point": {"x": 0.00003, "y": 0}},
          {"id": 3, "osm_id": "3", "line_ids": [2], "point": {"x": 0.00004, "y": 0}},
          {"id": 4, "osm_id": "4", "line_ids": [2], "point": {"x": 0.00005, "y": 0}}
        ],
        "lines": [
          {"id": 1, "interval": 60, "network_id": 1, "number": "1", "stop_ids": [1, 2],
           "type": "subway", "color": "red"},
          {"id": 2, "interval": 60, "network_id": 1, "number": "2", "stop_ids": [3, 4],
           "type": "bus", "color": "green"}
        ],
        "networks": [{"id": 1, "title": "Transit test"}]
      })",
                                {});
      FilesContainerW container(GetPlatform().WritableDir() + "TransitTest.mwm", FileWriter::OP_WRITE_EXISTING);
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
  StringsBundle const m_strings;
  std::map<std::string, m2::PointF> const m_symbols;
};

inline std::vector<routing::RouteSegment> MakeMixedSegments(std::vector<double> const & times,
                                                            bool withGatesAndTransfer)
{
  TEST_EQUAL(times.size(), 7, ());
  std::vector<routing::RouteSegment> segments;
  for (size_t i = 0; i < times.size(); ++i)
    segments.push_back(MakeSegment(i, times[i]));

  using namespace routing::transit;
  segments[2].SetTransitInfo(std::make_unique<routing::TransitInfo>(Edge(1, 2, 120, 1, false, std::vector<ShapeId>{})));
  segments[4].SetTransitInfo(std::make_unique<routing::TransitInfo>(Edge(3, 4, 120, 2, false, std::vector<ShapeId>{})));
  if (withGatesAndTransfer)
  {
    segments[1].SetTransitInfo(std::make_unique<routing::TransitInfo>(Gate(), true));
    segments[3].SetTransitInfo(
        std::make_unique<routing::TransitInfo>(Edge(2, 3, 40, kInvalidLineId, true, std::vector<ShapeId>{})));
    segments[5].SetTransitInfo(std::make_unique<routing::TransitInfo>(Gate(), false));
  }
  return segments;
}

inline TransitRouteInfo ProcessSubroutes(TransitRouteDisplay & display,
                                         std::vector<routing::RouteSegment> const & segments,
                                         std::vector<size_t> const & ends, size_t begin = 0)
{
  for (auto const end : ends)
  {
    TEST_LESS_OR_EQUAL(begin, end, ());
    TEST_LESS_OR_EQUAL(end, segments.size(), ());
    df::Subroute subroute;
    subroute.m_baseDistance = begin == 0 ? 0.0 : segments[begin - 1].GetDistFromBeginningMerc();
    subroute.m_polyline.Add(begin == 0 ? m2::PointD{} : segments[begin - 1].GetJunction().GetPoint());
    TEST(display.ProcessSubroute({segments.begin() + begin, segments.begin() + end}, subroute, end == segments.size()),
         (begin, end));
    begin = end;
  }
  TEST_EQUAL(begin, segments.size(), ());
  return display.GetRouteInfo();
}

inline int StepTimeSum(TransitRouteInfo const & info)
{
  return std::accumulate(info.m_steps.begin(), info.m_steps.end(), 0,
                         [](int total, TransitStepInfo const & step) { return total + step.m_timeInSec; });
}
}  // namespace transit_tests
