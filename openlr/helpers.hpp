#pragma once

#include "openlr/graph.hpp"

#include "routing/features_road_graph.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/timer.hpp"

#include <sstream>
#include <string>
#include <type_traits>

namespace openlr
{
inline bool PointsAreClose(m2::PointD const & p1, m2::PointD const & p2)
{
  double const kMwmRoadCrossingRadiusMeters = routing::GetRoadCrossingRadiusMeters();
  return MercatorBounds::DistanceOnEarth(p1, p2) < kMwmRoadCrossingRadiusMeters;
}

// TODO(mgsergio): Try to use double instead of uint32_t and leave whait is better.
inline uint32_t EdgeLength(Graph::Edge const & e)
{
  return static_cast<uint32_t>(MercatorBounds::DistanceOnEarth(e.GetStartPoint(), e.GetEndPoint()));
}

inline bool EdgesAraAlmostEqual(Graph::Edge const & e1, Graph::Edge const & e2)
{
  // TODO(mgsergio): Do I need to check fields other than points?
  return PointsAreClose(e1.GetStartPoint(), e2.GetStartPoint()) &&
         PointsAreClose(e1.GetEndPoint(), e2.GetEndPoint());
}

// TODO(mgsergio): Remove when unused.
inline std::string LogAs2GisPath(Graph::EdgeVector const & path)
{
  CHECK(!path.empty(), ("Paths should not be empty"));

  std::ostringstream ost;
  ost << "https://2gis.ru/moscow?queryState=";

  auto ll = MercatorBounds::ToLatLon(path.front().GetStartPoint());
  ost << "center%2F" << ll.lon << "%2C" << ll.lat << "%2F";
  ost << "zoom%2F" << 17 <<"%2F";
  ost << "ruler%2Fpoints%2F";
  for (auto const & e : path)
  {
    ll = MercatorBounds::ToLatLon(e.GetStartPoint());
    ost << ll.lon << "%20" << ll.lat << "%2C";
  }
  ll = MercatorBounds::ToLatLon(path.back().GetEndPoint());
  ost << ll.lon << "%20" << ll.lat;

  return ost.str();
}

inline std::string LogAs2GisPath(Graph::Edge const & e)
{
  return LogAs2GisPath(Graph::EdgeVector({e}));
}

template <
    typename T, typename U,
    typename std::enable_if<!(std::is_signed<T>::value ^ std::is_signed<U>::value), int>::type = 0>
typename std::common_type<T, U>::type AbsDifference(T const a, U const b)
{
  return a >= b ? a - b : b - a;
}

// TODO(mgsergio): Remove when/if unused.
class ScopedTimer : private my::Timer
{
public:
  ScopedTimer(std::chrono::milliseconds & ms) : m_ms(ms) {}
  ~ScopedTimer() { m_ms += TimeElapsedAs<std::chrono::milliseconds>(); }

private:
  std::chrono::milliseconds & m_ms;
};
}  // namespace openlr
