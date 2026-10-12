#pragma once

#include "map/nogps/roads.hpp"

#include "geometry/latlon.hpp"

#include <optional>

namespace nogps
{
/// Moves the car along its road, e.g. when the user corrects the position with the plus and minus buttons
/// without a route. The car never leaves the road: it follows the bends and stops at the first crossing,
/// where it may turn to another street.
class RoadWalker
{
public:
  // Short enough to follow the bends of a road.
  static double constexpr kStepM = 5;
  // The next point of the same road is close, a farther road is another one.
  static double constexpr kSnapRadiusM = 8;
  // A sharper bend within a step is a turn to another road.
  static double constexpr kMaxBendDeg = 45;
  // A crossing the car stands at does not stop it, otherwise it could never leave the crossing.
  static double constexpr kSameCrossingM = 1;
  // A crossing this close to the passed piece of the road is on the road.
  static double constexpr kCrossingOnRoadM = 3;
  // Crossings are searched around the start once, a bit farther than the distance.
  static double constexpr kCrossingSearchMarginM = 20;

  struct Result
  {
    ms::LatLon m_position;
    // Where the car looks at.
    double m_bearingDeg = 0;
    // Negative when moved back.
    double m_appliedM = 0;
    bool m_atCrossing = false;
  };

  /// \param bearingDeg the direction the car looks at.
  /// \param distanceM meters to move forward, negative to move back.
  /// \returns nothing if there is no road at the position.
  static std::optional<Result> Walk(Roads & roads, ms::LatLon const & position, double bearingDeg, double distanceM);
};
}  // namespace nogps
