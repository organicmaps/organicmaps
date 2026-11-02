#pragma once

#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace routing
{
namespace turns
{
namespace sound
{
/// Progress through the advance and immediate turn instructions.
enum class PronouncedNotification
{
  Nothing,
  First,
  Second,
};

std::string DebugPrint(PronouncedNotification notificationProgress);

/// Manages turn speech and the turn-after-next display state.
/// Generating turn speech requires initialized settings, locale and a relevant speed.
class NotificationManager
{
public:
  NotificationManager();

  static NotificationManager CreateNotificationManagerForTesting(
      uint32_t startBeforeSeconds, uint32_t minStartBeforeMeters, uint32_t maxStartBeforeMeters,
      uint32_t minDistToSayNotificationMeters, measurement_utils::Units lengthUnits, std::string const & engShortJson,
      uint32_t notificationTimeSecond, double speedMeterPerSecond);

  bool IsEnabled() const { return m_enabled; }
  void Enable(bool enable);
  std::string GetLocale() const { return m_getTtsText.GetLocale(); }
  void SetLengthUnits(measurement_utils::Units units);
  void SetSpeedMetersPerSecond(double speed);
  void SetLocale(std::string const & locale) { m_getTtsText.SetLocale(locale); }
  measurement_utils::Units GetLengthUnits() const { return m_settings.GetLengthUnits(); }
  void SetLocaleWithJsonForTesting(std::string const & json, std::string const & locale);

  /// \brief Generate text of route rebuild notification.
  std::string GenerateRecalculatingText() const;

  /// \brief Generate text of speed camera notification.
  std::string GenerateSpeedCameraText() const;

  /// Generate text of the track departure notification.
  std::string GenerateOffTrackText() const;

  /// Generate text of the initial track approach notification.
  std::string GenerateTrackApproachText() const;

  /// Updates turn speech and turn-after-next state. Turns must be ordered from nearest to farthest.
  /// Clears turnNotifications; emitted strings are nonempty. nextStreetInfo supplies spoken street names.
  /// A one-shot instruction is emitted only on the call that advances its notification state.
  void GenerateTurnNotifications(std::vector<TurnItemDist> const & turns, std::vector<std::string> & turnNotifications,
                                 RouteSegment::RoadNameInfo const & nextStreetInfo);

  /// Same notification behavior without spoken street names.
  void GenerateTurnNotifications(std::vector<TurnItemDist> const & turns, std::vector<std::string> & turnNotifications);

  /// Resets position-dependent state after creating a route or rerouting.
  void Reset();

  /// Returns the turn-after-next direction when eligible for display, or CarDirection::None.
  /// Eligibility requires approaching the nearest turn and a short distance between the two turns.
  /// For example, with "Turn right. Then turn left.", returns CarDirection::TurnLeft while approaching
  /// the right turn. Passing that turn clears its eligibility; the next pair is evaluated afresh.
  /// Once eligible, it remains visible until the nearest turn changes, even while speech is muted.
  CarDirection GetSecondTurnNotification() const { return m_secondTurnNotification; }

private:
  std::string GenerateTurnText(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
                               TurnItem const & turn, RouteSegment::RoadNameInfo const & nextStreetInfo) const;

  /// Generates turn sound notification for the nearest to the current position turn.
  std::string GenerateFirstTurnSound(TurnItem const & turn, double distanceToTurnMeters,
                                     RouteSegment::RoadNameInfo const & nextStreetInfo);

  /// Marks the advance instruction as handled without emitting speech.
  void FastForwardFirstTurnNotification();

  /// Computes turn-after-next eligibility independently of the speech toggle.
  CarDirection GenerateSecondTurnNotification(std::vector<TurnItemDist> const & turns);

  /// Disabled by default because the platform initializes the speech preference.
  bool m_enabled;

  /// Current nonnegative speed used to choose announcement distances.
  /// @TODO(bykoianko) Prefer an average over the last few seconds instead of the current speed.
  double m_speedMetersPerSecond;

  Settings m_settings;

  /// Progress for the nearest turn: Nothing before the advance instruction, First after it is
  /// emitted or skipped, Second after the immediate instruction. Reset when the nearest turn changes.
  PronouncedNotification m_nextTurnNotificationProgress;

  /// A combined instruction already covered the next turn's advance notification.
  /// For example, after "Turn right. Then turn left.", skip the advance instruction for the left turn
  /// when it becomes the nearest turn. This state persists between notification polls.
  bool m_turnNotificationWithThen;

  uint32_t m_nextTurnIndex;

  GetTtsText m_getTtsText;

  /// Turn-after-next direction for display, or CarDirection::None.
  CarDirection m_secondTurnNotification = CarDirection::None;

  /// Nearest-turn index for which the turn-after-next became visible; zero means unset.
  /// Zero is also a valid index at the route start, where eligibility is recomputed rather than latched.
  uint32_t m_secondTurnNotificationIndex;
};
}  // namespace sound
}  // namespace turns
}  // namespace routing
