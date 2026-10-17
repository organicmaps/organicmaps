#pragma once

#include "map/nogps/roads.hpp"

#include "geometry/latlon.hpp"

#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace nogps
{
/// Finds where the way the car has driven lies on the roads. The gyroscope and the speed tell the shape of the way
/// well, its place is told by the calculation. A ramp looping onto a viaduct, started tens of meters from where the
/// car is, is drawn beside the ramp, through the houses: the snapping finds no road around and after the loop takes
/// the nearest road going the car's way, on October 14 another road 12 m away, while the viaduct was 40 m away.
/// The way is moved as it is to the places on the roads going the car's way around, and where it lies on the roads
/// all along, there the car is.
class PathMatcher
{
public:
  // The way of the last meters is compared, a loop of a ramp with its approach.
  static double constexpr kWindowM = 150;
  // A shorter way is not told from the roads around it.
  static double constexpr kMinWindowM = 100;
  // Points of the way this far apart are checked.
  static double constexpr kCheckStepM = 10;
  // A point of the way is on a road this close going its way within the difference: the roads of the map are rough,
  // the way of the car is a few percent longer or shorter.
  static double constexpr kMaxRoadM = 8;
  static double constexpr kMaxRoadDiffDeg = 20;
  // A point or two of a long way may miss the rough roads of the map.
  static int constexpr kMaxMisses = 1;
  // A way missing the roads at fewer points is where the car is, wrong by meters along the road. The loop of
  // October 14 drawn beside the ramp missed the roads at 14 points of 15.
  static int constexpr kMinMissesToMove = 4;
  // The roads the car may be on are searched this far, along each of them the place is searched this far, by steps.
  static double constexpr kSearchM = 60;
  static double constexpr kSearchAlongM = 30;
  static double constexpr kSearchStepM = 10;
  // Places of the way this close are one: the step of the search.
  static double constexpr kSamePlaceM = 15;
  // The snapping chooses among the roads this close every second: a way lying on the roads that close to the car
  // is its road meters along, or another branch of a fork, which the heading tells. On October 11 the car was moved
  // 10 m to the other branch at the end of a turn around, and the road was lost.
  static double constexpr kMinMoveM = 20;

  /// Forgets the way driven: the car drives backwards, or it is not followed.
  void Reset() { m_history.clear(); }

  /// The car looks this way, e.g. the user has marked it on a road: the way driven is turned so. The calculated heading
  /// it was turned by before was wrong, before the start of the navigation it was none.
  void SetHeading(double headingDeg);

  /// \param yawDeltaDeg the clockwise rotation of the car measured by the gyroscope while it drove |distanceM|.
  /// \param headingDeg where the car looks after it by the calculation.
  void OnMotion(double yawDeltaDeg, double distanceM, double headingDeg);

  /// \param position where the car is by the calculation.
  /// \param why what is found, for the log.
  /// \returns where the car is if the way driven lies on the roads in one place, not around the calculated one.
  std::optional<RoadPoint> Match(Roads & roads, ms::LatLon const & position, std::string * why = nullptr) const;

  /// \returns at how many points the way driven misses the roads were it to end at |end|, nothing if the way is too
  /// short to tell. A place the car may be at after a turn that fits several, see InertialNavigator::kMaxMarks.
  std::optional<int> CountMissesAt(Roads & roads, ms::LatLon const & end) const;

private:
  struct Sample
  {
    double m_odometerM;
    double m_yawDeg;
    // The calculated heading: the way is turned as the car looked at its start.
    double m_headingDeg;
  };

  // A point of the way: meters east and north of its end, where the car looks there.
  struct Point
  {
    double m_eastM;
    double m_northM;
    double m_headingDeg;
  };

  // The points of the way every kCheckStepM from the end back, the end first.
  std::vector<Point> GetWay() const;

  // The points of the way not on the roads with the end at the position, counted up to |maxMisses| + 1.
  static int CountMisses(Roads & roads, ms::LatLon const & end, std::vector<Point> const & way, int maxMisses);

  std::deque<Sample> m_history;
  double m_odometerM = 0;
  double m_yawDeg = 0;
};
}  // namespace nogps
