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
/// \brief The SoundNotificationProgress enum represents which sound notifications
/// for turns were heard.
enum class SoundNotificationProgress
{
  NoNotificationsPronounced,
  BeforehandNotificationPronounced,
  SecondNotificationPronounced /** The second notification just before the turn was pronounced. */
};

string DebugPrint(SoundNotificationProgress const l);

/// \brief The TurnsSound class is responsible for all route turn sound notifications functionality.
/// To be able to generate turn sound notification the class needs to have correct Settings
/// and relevant speed.
class TurnsSound
{
  bool m_turnNotificationEnabled;
  /// In m_speedMetersPerSecond is intended for some speed which will be used for
  /// convertion a distance in seconds to distance in meters. It could be a current
  /// an end user speed or an avarage speed for last several seconds.
  /// @TODO(vbykoianko) It's better to use an avarage speed
  /// for last several seconds instead of the current speed here.
  double m_speedMetersPerSecond;
  Settings m_settings;
  mutable SoundNotificationProgress m_nextTurnNotificationProgress;
  mutable uint32_t m_nextTurnIndex;

public:
  TurnsSound() : m_turnNotificationEnabled(false) {}

  bool IsTurnNotificationEnabled() const { return m_turnNotificationEnabled; }
  void EnableTurnNotification(bool enable);
  void AssignSettings(Settings const & newSettings);
  void SetSpeedMetersPerSecond(double speed);

  void GetRouteFollowingInfo(location::FollowingInfo & info, TurnItem const & turn,
                             double distanceToTurnMeters) const;
  /// Reset states which reflects current route position.
  /// The method shall be called after creating a new route or after rerouting.
  void Reset();
};
}  // namespace sound
}  // namespace turns
}  // namespace routing
