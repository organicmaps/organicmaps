#pragma once

#include "routing/turns.hpp"
#include "routing/turns_sound_settings.hpp"

#include "std/string.hpp"

namespace location
{
class FollowingInfo;
}

namespace routing
{
namespace turns
{
namespace sound
{
/// \brief The PronouncedNotification enum represents which sound notifications
/// for turns were heard.
enum class PronouncedNotification
{
  Nothing,
  First,
  Second /** The second notification just before the turn was pronounced. */
};

string DebugPrint(PronouncedNotification const notificationProgress);

/// \brief The TurnsSound class is responsible for all route turn sound notifications functionality.
/// To be able to generate turn sound notification the class needs to have correct Settings
/// and relevant speed.
class TurnsSound
{
  bool m_enabled;
  /// In m_speedMetersPerSecond is intended for some speed which will be used for
  /// convertion a distance in seconds to distance in meters. It could be a current
  /// an end user speed or an average speed for last several seconds.
  /// @TODO(vbykoianko) It's better to use an average speed
  /// for last several seconds instead of the current speed here.
  double m_speedMetersPerSecond;
  Settings m_settings;
  PronouncedNotification m_nextNotificationProgress;
  uint32_t m_nextTurnIndex;

public:
  TurnsSound() : m_enabled(false), m_speedMetersPerSecond(0.), m_settings(),
      m_nextNotificationProgress(PronouncedNotification::Nothing), m_nextTurnIndex(0) {}

  bool IsEnabled() const { return m_enabled; }
  void Enable(bool enable);
  void SetSettings(Settings const & newSettings);
  void SetSpeedMetersPerSecond(double speed);

   /// \brief UpdateRouteFollowingInfo updates information about the next turn notification.
   /// It also fills FollowingInfo::m_turnNotifications when it's necessary.
   /// If this TurnsSound wants to play a sound message once it should push one item to
   /// the vector FollowingInfo::m_turnNotifications once when UpdateRouteFollowingInfo is called.
   /// \param info is a parameter to fill info.m_turnNotifications
   /// \param turn contains information about the next turn.
   /// \param distanceToTurnMeters is distance to the next turn in meters.
  void UpdateRouteFollowingInfo(location::FollowingInfo & info, TurnItem const & turn,
                                double distanceToTurnMeters);
  /// Reset states which reflects current route position.
  /// The method shall be called after creating a new route or after rerouting.
  void Reset();
};
}  // namespace sound
}  // namespace turns
}  // namespace routing
