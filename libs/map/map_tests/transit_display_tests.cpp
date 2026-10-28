#include "testing/testing.hpp"

#include "map/transit/transit_display.hpp"
#include "map/transit/transit_reader.hpp"

#include "drape_frontend/route_shape.hpp"

#include "routing/route.hpp"

#include "indexer/data_source.hpp"

#include "platform/duration.hpp"

#include "base/strings_bundle.hpp"

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
    TransitRouteDisplay display(reader, getMwmId, getStrings, nullptr /* bmManager */, symbols);
    geometry::PointWithAltitude const start({0, 0}, 0);
    geometry::PointWithAltitude const middle({0.001, 0}, 0);
    geometry::PointWithAltitude const finish({0.002, 0}, 0);
    std::vector<routing::RouteSegment> segments;
    for (size_t i = 0; i < 2; ++i)
    {
      segments.emplace_back(routing::Segment(0, 0, i, true), routing::turns::TurnItem(), i == 0 ? middle : finish,
                            routing::RouteSegment::RoadNameInfo());
      segments.back().SetDistancesAndTime((i + 1) * 100, (i + 1) * 0.001, i == 0 ? test.m_timeSec / 2 : test.m_timeSec);
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
      TEST_EQUAL(display.GetRouteInfo().m_totalTimeInSec, static_cast<int>(segments.back().GetTimeFromBeginningSec()),
                 (test.m_timeSec, i));
    }

    auto const & info = display.GetRouteInfo();
    TEST_EQUAL(info.m_totalTimeInSec, test.m_expectedSeconds, ());
    TEST_EQUAL(platform::Duration(info.m_totalTimeInSec).GetRoutingString(),
               platform::Duration(route.GetTotalTimeSec()).GetRoutingString(), ());
  }
}
}  // namespace transit_display_tests
