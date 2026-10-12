#pragma once

#include "map/nogps/roads.hpp"

#include "geometry/latlon.hpp"

#include <deque>
#include <optional>

namespace nogps
{
/// Finds how far the calculated position is ahead of the car or behind it along the road: the car turns where
/// the road bends, so the rotation measured by the gyroscope is compared with the shape of the road around the
/// position. Bends are much more frequent than turns at crossings, where the distance error is removed too.
/// Changing a lane rotates the car by a few degrees and back, it doesn't look like a bend.
class BendMatcher
{
public:
  // The rotation of the car is compared with the road along this distance.
  static double constexpr kWindowM = 150;
  static double constexpr kStepM = 5;
  // A straighter road tells nothing about the position along it.
  static double constexpr kMinBendDeg = 20;
  // The shape of the road on the map is rough, a larger difference is another road, e.g. after a fork.
  static double constexpr kMaxDiffDeg = 4;
  // The match is clear if the positions this far aside of the best one fit the road much worse. On a
  // roundabout or a long even bend they fit the same.
  static double constexpr kClearAsideM = 15;
  static double constexpr kClearRatio = 2;
  static double constexpr kMinAsideDiffDeg = 1;

  /// Forgets the way driven: the car was moved to another road, or a turn at a crossing is not a bend.
  void Reset() { m_history.clear(); }

  /// \param yawDeltaDeg the clockwise rotation of the car measured by the gyroscope while it drove |distanceM|.
  void OnMotion(double yawDeltaDeg, double distanceM);

  /// \param position the calculated position on a road.
  /// \param bearingDeg where the car looks at.
  /// \param maxShiftM the largest error to look for.
  /// \returns how far the calculated position is ahead of the car along the road, negative if it is behind.
  /// Nothing if it is not known: the road is straight, or its shape doesn't tell the place clearly.
  std::optional<double> Match(Roads & roads, ms::LatLon const & position, double bearingDeg, double maxShiftM) const;

private:
  struct Sample
  {
    double m_odometerM;
    double m_yawDeg;
  };

  double YawAt(double odometerM) const;

  // The last kWindowM and a little more.
  std::deque<Sample> m_history;
  double m_odometerM = 0;
  double m_yawDeg = 0;
};
}  // namespace nogps
