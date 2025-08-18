#pragma once

#include "geometry/latlon.hpp"

#include "platform/distance.hpp"

#include "routing/turns.hpp"
#include "routing/turns_sound_settings.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace routing
{
class FollowingInfo
{
public:
  FollowingInfo()
    : m_turn(turns::CarDirection::None)
    , m_nextTurn(turns::CarDirection::None)
    , m_exitNum(0)
    , m_time(0)
    , m_completionPercent(0)
    , m_pedestrianTurn(turns::PedestrianDirection::None)
  {}

  // SingleLaneInfoClient is used for passing information about a lane to client platforms such as
  // Android, iOS and so on.
  struct SingleLaneInfoClient
  {
    std::vector<int8_t> m_lane;  // Possible directions for the lane.
    bool m_isRecommended;        // m_isRecommended is true if the lane is recommended for a user.

    explicit SingleLaneInfoClient(turns::SingleLaneInfo const & singleLaneInfo)
      : m_isRecommended(singleLaneInfo.m_isRecommended)
    {
      turns::TSingleLane const & lane = singleLaneInfo.m_lane;
      m_lane.resize(lane.size());
      std::transform(lane.cbegin(), lane.cend(), m_lane.begin(),
                     [](turns::LaneWay l) { return static_cast<int8_t>(l); });
    }
  };

  bool IsValid() const { return m_distToTarget.IsValid(); }

  /// @name Formatted covered distance.
  platform::Distance m_distToTarget;

  /// @name Formatted distance to the next turn.
  //@{
  platform::Distance m_distToTurn;
  turns::CarDirection m_turn;
  /// Turn after m_turn. Returns NoTurn if there is no turns after.
  turns::CarDirection m_nextTurn;
  uint32_t m_exitNum;
  //@}
  int m_time;
  // m_lanes contains lane information on the edge before the turn.
  std::vector<SingleLaneInfoClient> m_lanes;
  // m_turnNotifications contains information about the next turn notifications.
  // If there is nothing to pronounce m_turnNotifications is empty.
  // If there is something to pronounce the size of m_turnNotifications may be one or even more
  // depends on the number of notifications to prononce.
  std::vector<std::string> m_turnNotifications;
  // Current street name. May be empty.
  std::string m_currentStreetName;
  // The next street name. May be empty.
  std::string m_nextStreetName;
  // The next next street name. May be empty.
  std::string m_nextNextStreetName;

  // Percentage of the route completion.
  double m_completionPercent;

  /// @name Pedestrian direction information
  //@{
  turns::PedestrianDirection m_pedestrianTurn;
  //@}

  // Current speed limit in meters per second.
  // If no info about speed limit then m_speedLimitMps < 0.
  double m_speedLimitMps = -1.0;
};
}  // namespace routing
