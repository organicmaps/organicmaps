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
/// \brief The PronouncedNotification enum represents which sound notifications
/// for turns were heard.
enum class PronouncedNotification
{
  Nothing,
  First,
  Second, /** The second notification just before the turn was pronounced. */
};

std::string DebugPrint(PronouncedNotification notificationProgress);

/// \brief The TurnsSound class is responsible for all route turn sound notifications functionality.
/// To be able to generate turn sound notification the class needs to have correct Settings
/// and relevant speed.
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

  /// \brief GenerateTurnNotifications updates information about the next turn notification.
  /// It also fills turnNotifications when it's necessary.
  /// If this TurnsSound wants to play a sound message once it should push one item to
  /// the vector turnNotifications once when GenerateTurnNotifications is called.
  /// \param turns contains information about the next turns starting from the closest one.
  /// \param distanceToTurnMeters is distance to the next turn in meters.
  /// \param turnNotifications is a parameter to fill it if it's necessary.
  /// \param nextStreetInfo is the RoadNameInfo for the next street to turn on (optional)
  /// \note The client implies turnNotifications does not contain empty strings.
  void GenerateTurnNotifications(std::vector<TurnItemDist> const & turns, std::vector<std::string> & turnNotifications,
                                 RouteSegment::RoadNameInfo const & nextStreetInfo);

  /// \brief GenerateTurnNotifications updates information about the next turn notification.
  /// It also fills turnNotifications when it's necessary.
  /// If this TurnsSound wants to play a sound message once it should push one item to
  /// the vector turnNotifications once when GenerateTurnNotifications is called.
  /// \param turns contains information about the next turns starting from the closest one.
  /// \param distanceToTurnMeters is distance to the next turn in meters.
  /// \param turnNotifications is a parameter to fill it if it's necessary.
  /// \note The client implies turnNotifications does not contain empty strings.
  void GenerateTurnNotifications(std::vector<TurnItemDist> const & turns, std::vector<std::string> & turnNotifications);

  /// Reset states which reflects current route position.
  /// The method shall be called after creating a new route or after rerouting.
  void Reset();

  /// Returns the second closest turn after its "Then" notification is issued, or a turn with
  /// CarDirection::None when it should not be displayed. This state is independent of m_enabled.
  TurnItem const & GetSecondTurn() const { return m_secondTurnNotification; }

private:
  std::string GenerateTurnText(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
                               TurnItem const & turn, RouteSegment::RoadNameInfo const & nextStreetInfo) const;

  /// Generates turn sound notification for the nearest to the current position turn.
  std::string GenerateFirstTurnSound(TurnItem const & turn, double distanceToTurnMeters,
                                     RouteSegment::RoadNameInfo const & nextStreetInfo);

  /// Changes the state of the class to emulate that first turn notification is pronounced
  /// without pronunciation.
  void FastForwardFirstTurnNotification();

  /// Updates m_secondTurnNotification from turns, ordered closest first.
  void GenerateSecondTurnNotification(std::vector<TurnItemDist> const & turns);

  /// m_enabled == true when tts is turned on.
  /// Important! Clients (iOS/Android) implies that m_enabled is false by default.
  bool m_enabled;

  /// In m_speedMetersPerSecond is intended for some speed which will be used for
  /// conversion a distance in seconds to distance in meters. It could be a current
  /// an end user speed or an average speed for last several seconds.
  /// @TODO(bykoianko) It's better to use an average speed
  /// for last several seconds instead of the current speed here.
  double m_speedMetersPerSecond;

  Settings m_settings;

  /// m_nextTurnNotificationProgress keeps a status which is being changing while
  /// an end user is coming to the closest (the next) turn along the route.
  /// When an end user is far from the next turn
  /// m_nextTurnNotificationProgress == Nothing.
  /// After the first turn notification has been pronounced
  /// m_nextTurnNotificationProgress == First.
  /// After the second notification has been pronounced
  /// m_nextTurnNotificationProgress == Second.
  PronouncedNotification m_nextTurnNotificationProgress;

  /// The flag is set to true if notification about the second turn was pronounced.
  /// It could happen in expressions like "Turn right. Then turn left."
  /// This flag is used to pass the information if second turn notification was pronounced
  /// between two calls of GenerateTurnNotifications() method.
  bool m_turnNotificationWithThen;

  uint32_t m_nextTurnIndex;

  /// getTtsText is a convector form turn notification information and locale to
  /// notification string.
  GetTtsText m_getTtsText;

  /// The turn after the closest one when it is time to display it, or an empty turn otherwise.
  TurnItem m_secondTurnNotification;

  /// m_secondTurnNotificationIndex is an index of the closest turn on the route polyline
  /// where m_secondTurnNotification was last populated.
  /// If the closest turn is changed m_secondTurnNotification is cleared.
  /// \note 0 is a valid index. But in this context it could be considered as invalid
  /// because if firstTurnIndex == 0 that means we're at very beginning of the route
  /// and we're about to making a turn. In that case it's no use to inform a user about
  /// the turn after the next one.
  uint32_t m_secondTurnNotificationIndex;
};
}  // namespace sound
}  // namespace turns
}  // namespace routing
