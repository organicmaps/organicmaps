#pragma once

#include "routing/routing_quality/api/api.hpp"

#include "base/visitor.hpp"

#include <string>
#include <vector>

namespace routing_quality
{
namespace api
{
namespace google
{
struct LatLon
{
  DECLARE_VISITOR(visitor(m_lat, "lat"), visitor(m_lon, "lng"))

  bool operator==(LatLon const & rhs) const;

  double m_lat = 0.0;
  double m_lon = 0.0;
};

struct Step
{
  DECLARE_VISITOR(visitor(m_startLocation, "start_location"), visitor(m_endLocation, "end_location"))

  bool operator==(Step const & rhs) const;

  LatLon m_startLocation;
  LatLon m_endLocation;
};

struct Duration
{
  DECLARE_VISITOR(visitor(m_seconds, "value"))

  double m_seconds = 0.0;
};

struct Distance
{
  DECLARE_VISITOR(visitor(m_meters, "value"))

  double m_meters = 0.0;
};

struct Leg
{
  DECLARE_VISITOR(visitor(m_distance, "distance"), visitor(m_duration, "duration"), visitor(m_steps, "steps"))

  Distance m_distance;
  Duration m_duration;
  std::vector<Step> m_steps;
};

struct Route
{
  DECLARE_VISITOR(visitor(m_legs, "legs"))

  std::vector<Leg> m_legs;
};

struct GoogleResponse
{
  DECLARE_VISITOR(visitor(m_routes, "routes"))

  ResultCode m_code = ResultCode::Error;
  std::vector<Route> m_routes;
};

std::string DebugPrint(LatLon const & latlon);
}  // namespace google
}  // namespace api
}  // namespace routing_quality
