#pragma once

#include "geometry/latlon.hpp"

#include <optional>
#include <vector>

namespace nogps
{
struct RoadPoint
{
  ms::LatLon m_point;
  // Clockwise from the north, any of the two directions of the road.
  double m_bearingDeg = 0;
};

/// The roads of the map, the car is always on one of them.
class Roads
{
public:
  virtual ~Roads() = default;

  /// \param bearingDeg the direction the car looks at, a road going that way is preferred.
  /// \returns the closest point of a road within |radiusM|.
  virtual std::optional<RoadPoint> Snap(ms::LatLon const & point, std::optional<double> bearingDeg, double radiusM) = 0;

  /// \returns the closest point of every road within |radiusM| the car may drive along, the roads Snap() chooses
  /// from, in no order. Where a road forks, these are its branches. The followed route is not preferred, a
  /// one-way road going against the car is left out.
  virtual std::vector<RoadPoint> SnapAll(ms::LatLon const & point, double bearingDeg, double radiusM)
  {
    if (auto const road = Snap(point, bearingDeg, radiusM))
      return {*road};
    return {};
  }

  /// \returns the points where three or more roads meet within |radiusM|.
  virtual std::vector<ms::LatLon> FindCrossings(ms::LatLon const & center, double radiusM) = 0;

  /// \returns true if a roundabout is within |radiusM|.
  virtual bool IsRoundaboutNear(ms::LatLon const & /* center */, double /* radiusM */) { return false; }
};
}  // namespace nogps
