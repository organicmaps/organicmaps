#include "testing/testing.hpp"

#include "map/transit/transit_display.hpp"
#include "map/transit/transit_reader.hpp"

#include "drape_frontend/route_shape.hpp"

#include "routing/route.hpp"

#include "indexer/data_source.hpp"

#include "platform/duration.hpp"

#include "base/strings_bundle.hpp"

#include <algorithm>
#include <vector>

namespace transit_display_tests
{
UNIT_TEST(TransitDisplay_TotalUsesRouteDuration)
{
  FrozenDataSource dataSource;
  TransitReadManager reader(dataSource, [](FeatureCallback const &, std::vector<FeatureID> const &) {},
                            [](m2::RectD const &) { return std::vector<MwmSet::MwmId>(); });
  StringsBundle const strings;
  std::map<std::string, m2::PointF> const symbols;
  auto const getMwmId = [](routing::NumMwmId) { return MwmSet::MwmId(); };
  auto const getStrings = [&strings]() -> StringsBundle const & { return strings; };

  struct Case
  {
    double m_timeSec;
    int m_expectedSeconds;
  };
  Case const cases[] = {{0.9, 0},       {20.75, 20},    {59.99, 59},     {1649.5, 1649},
                        {1665.0, 1665}, {2085.0, 2085}, {94485.0, 94485}};
  for (auto const & test : cases)
  {
    auto const timeSec = test.m_timeSec;
    TransitRouteDisplay display(reader, getMwmId, getStrings, nullptr /* bmManager */, symbols);
    geometry::PointWithAltitude const start({0, 0}, 0);
    geometry::PointWithAltitude const middle({0.001, 0}, 0);
    geometry::PointWithAltitude const finish({0.002, 0}, 0);
    std::vector<routing::RouteSegment> segments;
    for (size_t i = 0; i < 2; ++i)
    {
      segments.emplace_back(routing::Segment(0, 0, i, true), routing::turns::TurnItem(), i == 0 ? middle : finish,
                            routing::RouteSegment::RoadNameInfo());
      segments.back().SetDistancesAndTime((i + 1) * 100, (i + 1) * 0.001,
                                          i == 0 ? std::min(120.2, timeSec / 2) : timeSec);
    }
    routing::Route route;
    route.SetRouteSegments(std::move(segments));
    route.SetSubroutes(std::vector<routing::Route::SubrouteAttrs>{{start, middle, 0, 1}, {middle, finish, 1, 2}});

    for (size_t i = 0; i < route.GetSubrouteCount(); ++i)
    {
      route.GetSubrouteInfo(i, segments);
      df::Subroute subroute;
      subroute.m_polyline.Add(route.GetSubrouteAttrs(i).GetStart().GetPoint());
      TEST(display.ProcessSubroute(segments, subroute), ());
    }

    auto const & info = display.GetRouteInfo(route.GetTotalTimeSec());
    TEST_EQUAL(info.m_totalTimeInSec, test.m_expectedSeconds, ());
    TEST_EQUAL(platform::Duration(info.m_totalTimeInSec).GetRoutingString(),
               platform::Duration(static_cast<unsigned long>(route.GetTotalTimeSec())).GetRoutingString(), ());
    TEST_EQUAL(info.m_steps.size(), 3, ());
    TEST_EQUAL(info.m_steps[1].m_type, TransitType::IntermediatePoint, ());
    if (timeSec == 1649.5)
    {
      auto const minute = platform::Duration::GetUnitsString(platform::Duration::Units::Minutes);
      TEST_EQUAL(platform::Duration(info.m_totalTimeInSec).GetRoutingString(),
                 "27" + platform::kNonBreakingSpace + minute, ());
      TEST_EQUAL(info.m_steps[0].m_timeInSec, 121, ());
      TEST_EQUAL(info.m_steps[2].m_timeInSec, 1529, ());
    }
  }
}
}  // namespace transit_display_tests
