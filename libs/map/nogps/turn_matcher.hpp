#pragma once

#include "map/nogps/roads.hpp"

#include "geometry/latlon.hpp"

#include <optional>
#include <string>

namespace nogps
{
/// Finds the crossing the car has turned at. The gyroscope tells the turn exactly, but the calculated
/// distance lags behind or runs ahead of the car, so the calculated turn is before or after the real crossing
/// and the car would leave the road after it. The real crossing is the one on the road the car has driven
/// along with a road going the new way.
class TurnMatcher
{
public:
  // Smaller turns are bends of the road.
  static double constexpr kMinTurnDeg = 35;
  // Larger turns are turns around, not at a crossing.
  static double constexpr kMaxTurnDeg = 150;
  // The crossing is searched this far along the road at least. Not farther than the calculated distance may
  // be wrong by: the turn was moved to another crossing 57 m away with the car 15 m from the right one.
  static double constexpr kMinSearchM = 25;
  // Nor farther than this, however long the car has driven without a known place: the bends of the roads keep
  // the calculated distance right within 10 m, and no turn moved farther than 20 m was moved to its crossing.
  static double constexpr kMaxSearchM = 60;
  // A crossing farther from the road the car drove along is on a parallel street.
  static double constexpr kMaxSideOffsetM = 20;
  // The roads of a crossing are checked this far from it: close to it all its roads meet.
  static double constexpr kRoadCheckDistanceM = 12;
  static double constexpr kRoadCheckRadiusM = 6;
  static double constexpr kMaxRoadDiffDeg = 35;
  // The road the car was on going the new way within this difference has bent, the car has followed it.
  static double constexpr kMaxBendEndDiffDeg = 25;
  // The crossings this close to a turn are written to the trip log, a few closest ones: farther than they are
  // searched, to see on real trips where the crossing of a turn that was not moved to one really was.
  static double constexpr kLogRadiusM = 80;
  static size_t constexpr kLogCrossings = 6;

  /// \param corner where the car has turned by the calculation.
  /// \param fromBearingDeg where the car looked before the turn.
  /// \param toBearingDeg where the car looks after the turn.
  /// \param searchM how far along the road the crossing is searched, e.g. the accuracy of the calculation.
  /// \param whyNot why the turn is not at a crossing, for the log.
  /// \returns the crossing, or nothing if the turn is not at a crossing.
  static std::optional<ms::LatLon> FindCrossing(Roads & roads, ms::LatLon const & corner, double fromBearingDeg,
                                                double toBearingDeg, double searchM, std::string * whyNot = nullptr);

  /// \returns the crossings around the corner for the trip log, the closest first, as "+40/-3/BA,-60/+0/B-": how
  /// far the crossing is ahead of the corner the way the car came, how far to the right, meters, whether it has the
  /// road the car came by (B) and a road going the new way (A). "-" if there are no crossings around.
  static std::string DescribeCrossings(Roads & roads, ms::LatLon const & corner, double fromBearingDeg,
                                       double toBearingDeg);

  /// Follows the road the car was on from the corner: the road going on the way the car looked before. At a
  /// crossing it goes straight on or ends.
  /// \returns true if the road bends to the new direction within the distance.
  static bool IsBend(Roads & roads, ms::LatLon const & position, double fromBearingDeg, double toBearingDeg,
                     double distanceM);
};
}  // namespace nogps
