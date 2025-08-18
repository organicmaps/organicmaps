#pragma once

#include "routing/road_point.hpp"

#include <cstdint>

namespace routing
{
class RoutePoint final
{
public:
  RoutePoint() = default;
  RoutePoint(RoadPoint const & rp, double time) : m_roadPoint(rp), m_time(time) {}
  RoutePoint(uint32_t featureId, uint32_t pointId, double time) : m_roadPoint(featureId, pointId), m_time(time) {}

  RoadPoint const & GetRoadPoint() const { return m_roadPoint; }
  double GetTime() const { return m_time; }

private:
  RoadPoint m_roadPoint;
  // time in seconds from start to arrival to point.
  double m_time = 0.0;
};
}  // namespace routing
