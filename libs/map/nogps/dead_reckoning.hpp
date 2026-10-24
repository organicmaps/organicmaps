#pragma once

#include "geometry/latlon.hpp"

namespace nogps
{
/// Moves the position by the car speed along the heading.
/// The accuracy degrades with the distance driven since the last reference position (GPS, a manual mark),
/// mostly because of the heading error.
class DeadReckoning
{
public:
  static double constexpr kBaseAccuracyM = 15;
  static double constexpr kAccuracyPerMeter = 0.02;
  // A road going in a different direction is not the road the car is on (or the car is turning).
  static double constexpr kMaxSnapHeadingDiffDeg = 30;
  // The heading is pulled to the road slowly and only when they nearly agree: it removes the slow gyroscope
  // drift only. A larger difference is a real turn or a road going away from the car, e.g. a fork or a part of a
  // roundabout behind the car, where the gyroscope is right and the road is not.
  static double constexpr kMaxHeadingPullDiffDeg = 10;
  static double constexpr kSnapHeadingWeight = 0.1;
  // A heading aside from the road the car keeps being snapped to is wrong, e.g. taken from a short piece of a road
  // at a crossing: a fork or a slow turn takes the car off the road sooner.
  static double constexpr kMaxAsideOnRoadM = 50;
  // A car turning after the crossing it was put on may be taking a branch there: the road is not trusted any more.
  static double constexpr kMaxTrustedRotationDeg = 10;

  void SetPosition(ms::LatLon const & position);
  /// Moves the position without making it a reference one: the accuracy keeps degrading.
  void MoveTo(ms::LatLon const & position);
  /// \param headingDeg clockwise from the north.
  void SetHeading(double headingDeg);
  /// The road the car was just put on is the right one, e.g. at a crossing the turn is moved to: for the next
  /// kMaxAsideOnRoadM the heading is pulled to it at once, a difference up to kMaxSnapHeadingDiffDeg is not a fork,
  /// until the car turns by more than kMaxTrustedRotationDeg.
  void TrustRoad();
  void SetSpeed(double speedMps);
  /// \param deltaDeg clockwise rotation.
  void Rotate(double deltaDeg);
  /// Moves the position along the heading with the current speed.
  void Advance(double dtSec);
  /// Moves the position forward by the distance.
  void AdvanceBy(double distanceM);
  /// Moves the position back by the distance: the car drives backwards, it looks where it did.
  void ReverseBy(double distanceM);

  /// Pulls the position to the road the car is on: it removes the side error. The heading is corrected
  /// towards the road direction, it removes the gyroscope drift on straight roads.
  /// \param roadBearingDeg direction of the road, the opposite direction is the same road.
  /// \param pullHeading false where the road forks: the heading tells the branch there, and a branch taken by
  /// mistake must not turn it.
  /// \returns false if the road goes in another direction and it is ignored.
  bool SnapToRoad(ms::LatLon const & road, double roadBearingDeg, bool pullHeading = true);

  bool HasPosition() const { return m_hasPosition; }
  bool HasHeading() const { return m_hasHeading; }
  bool IsReady() const { return m_hasPosition && m_hasHeading; }
  ms::LatLon const & GetPosition() const { return m_position; }
  double GetHeading() const { return m_headingDeg; }
  double GetSpeed() const { return m_speedMps; }
  double GetDistanceSinceFix() const { return m_distanceSinceFixM; }
  /// How many times the position has been set: what was told about it before is not about the new one.
  int GetFixCount() const { return m_fixCount; }
  double GetAccuracy() const { return kBaseAccuracyM + kAccuracyPerMeter * m_distanceSinceFixM; }

private:
  bool m_hasPosition = false;
  bool m_hasHeading = false;
  ms::LatLon m_position;
  double m_headingDeg = 0;
  double m_speedMps = 0;
  double m_distanceSinceFixM = 0;
  int m_fixCount = 0;
  double m_odometerM = 0;
  // The heading went aside from the road to the side of |m_asideSign| at this odometer.
  bool m_isAside = false;
  double m_asideSinceM = 0;
  double m_asideSign = 0;
  double m_trustedUntilM = 0;
  double m_trustedRotationDeg = 0;
};
}  // namespace nogps
