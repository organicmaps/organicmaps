#pragma once

#include "platform/distance.hpp"

#include "indexer/road_shields_parser.hpp"

#include "routing/lanes/lane_info.hpp"
#include "routing/turns.hpp"

#include <string>
#include <vector>

namespace routing
{
class FollowingInfo
{
public:
  struct RoadShieldInfo
  {
    /// An array of road shields for the target street
    ftypes::RoadShieldsSetT m_targetRoadShields;
    /// Position of the road shields in the street name string. [Inclusive, Exclusive)
    std::pair<uint16_t, uint16_t> m_targetRoadShieldsPosition;
    /// An array of junction info shields for the target street
    ftypes::RoadShieldsSetT m_junctionInfo;
    /// Position of the junction info in the street name string. [Inclusive, Exclusive)
    std::pair<uint16_t, uint16_t> m_junctionInfoPosition;
  };

  FollowingInfo() = default;

  bool IsValid() const { return m_distToTarget.IsValid(); }

  /// @name Formatted covered distance.
  platform::Distance m_distToTarget;

  /// @name Next turns and formatted distance to the closest one.
  //@{
  platform::Distance m_distToTurn;
  turns::CarDirection m_turn = turns::CarDirection::None;
  /// Exit number and roundabout details of m_turn.
  uint32_t m_exitNum = 0;
  turns::RoundaboutInfo m_roundaboutInfo;
  /// Turn after m_turn, or CarDirection::None when it should not be displayed.
  turns::CarDirection m_nextTurn = turns::CarDirection::None;
  /// Exit number and roundabout details of m_nextTurn.
  uint32_t m_nextExitNum = 0;
  turns::RoundaboutInfo m_nextRoundaboutInfo;
  //@}
  int m_time = 0;
  /// Contains lane information on the edge before the turn.
  turns::lanes::LanesInfo m_lanes;
  // m_turnNotifications contains information about the next turn notifications.
  // If there is nothing to pronounce m_turnNotifications is empty.
  // If there is something to pronounce the size of m_turnNotifications may be one or even more
  // depends on the number of notifications to prononce.
  std::vector<std::string> m_turnNotifications;
  // Current street name. May be empty.
  std::string m_currentStreetName;
  // Road shields for the current street.
  RoadShieldInfo m_currentStreetShields;
  // The next street name. May be empty.
  std::string m_nextStreetName;
  // Road shields for the next street.
  RoadShieldInfo m_nextStreetShields;
  // The next next street name. May be empty.
  std::string m_nextNextStreetName;
  // Road shields for the next next street.
  RoadShieldInfo m_nextNextStreetShields;

  // Percentage of the route completion.
  double m_completionPercent = 0.0;

  /// @name Pedestrian direction information
  //@{
  turns::PedestrianDirection m_pedestrianTurn = turns::PedestrianDirection::None;
  //@}

  // Current speed limit in meters per second.
  // If no info about speed limit then m_speedLimitMps < 0.
  double m_speedLimitMps = -1.0;
};
}  // namespace routing
