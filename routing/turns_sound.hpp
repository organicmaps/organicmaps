#pragma once

#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include "platform/settings.hpp"

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
  friend void UnitTest_TurnsSoundMetersTest();
  friend void UnitTest_TurnsSoundMetersTwoTurnsTest();
  friend void UnitTest_TurnsSoundFeetTest();
  friend void UnitTest_TurnsSoundComposedTurnTest();
  friend void UnitTest_TurnsSoundRoundaboutTurnTest();

  /// \brief The private contructor is used only for testing.
  TurnsSound(uint32_t startBeforeSeconds, uint32_t minStartBeforeMeters,
             uint32_t maxStartBeforeMeters, uint32_t minDistToSayNotificationMeters)
    : m_enabled(false), m_speedMetersPerSecond(0.), m_settings(),
      m_nextTurnNotificationProgress(PronouncedNotification::Nothing),
      m_turnNotificationWithThen(false),  m_nextTurnIndex(0),
      m_startBeforeSeconds(startBeforeSeconds), m_minStartBeforeMeters(minStartBeforeMeters),
      m_maxStartBeforeMeters(maxStartBeforeMeters),
      m_minDistToSayNotificationMeters(minDistToSayNotificationMeters) {}

  /// m_enabled == true when tts is turned on.
  /// Important! Clients (iOS/Android) implies that m_enabled is false by default.
  bool m_enabled;
  /// In m_speedMetersPerSecond is intended for some speed which will be used for
  /// convertion a distance in seconds to distance in meters. It could be a current
  /// an end user speed or an average speed for last several seconds.
  /// @TODO(vbykoianko) It's better to use an average speed
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
  /// between two calls of GenerateTurnSound() method.
  bool m_turnNotificationWithThen;
  uint32_t m_nextTurnIndex;
  /// getTtsText is a convector form turn notification information and locale to
  /// notification string.
  GetTtsText m_getTtsText;

  string GenerateTurnText(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
                          TurnDirection turnDir, ::Settings::Units lengthUnits) const;
  /// Generates turn sound notification for the nearest to the current position turn.
  string GenerateFirstTurnSound(TurnItem const & turn, double distanceToTurnMeters);
  /// Changes the state of the class to emulate that first turn notification is pronouned
  /// without pronunciation.
  void FastForwardFirstTurnNotification();

  // To inform an end user about the next turn with the help of an voice information message
  // an operation system needs:
  // - to launch TTS subsystem;
  // - to pronounce the message.
  // So to inform the user in time it's necessary to start
  // m_startBeforeSeconds before the time. It is used in the following way:
  // we start playing voice notice in m_startBeforeSeconds * TurnsSound::m_speedMetersPerSecond
  // meters before the turn (for the second voice notification).
  // When m_startBeforeSeconds * TurnsSound::m_speedMetersPerSecond  is too small or too large
  // we use m_minStartBeforeMeters or m_maxStartBeforeMeters correspondingly.
  uint32_t const m_startBeforeSeconds;
  uint32_t const m_minStartBeforeMeters;
  uint32_t const m_maxStartBeforeMeters;

  // m_minDistToSayNotificationMeters is minimum distance between two turns
  // when pronouncing the first notification about the second turn makes sense.
  uint32_t const m_minDistToSayNotificationMeters;

public:
  TurnsSound() : m_enabled(false), m_speedMetersPerSecond(0.), m_settings(),
      m_nextTurnNotificationProgress(PronouncedNotification::Nothing),
      m_turnNotificationWithThen(false),  m_nextTurnIndex(0),
      m_startBeforeSeconds(5), m_minStartBeforeMeters(25), m_maxStartBeforeMeters(150),
      m_minDistToSayNotificationMeters(170) {}

  bool IsEnabled() const { return m_enabled; }
  void Enable(bool enable);
  void SetLengthUnits(::Settings::Units units);
  inline ::Settings::Units GetLengthUnits() const { return m_settings.GetLengthUnits(); }
  inline void SetLocale(string const & locale) { m_getTtsText.SetLocale(locale); }
  inline string GetLocale() const { return m_getTtsText.GetLocale(); }
  void SetSpeedMetersPerSecond(double speed);

   /// \brief GenerateTurnSound updates information about the next turn notification.
   /// It also fills turnNotifications when it's necessary.
   /// If this TurnsSound wants to play a sound message once it should push one item to
   /// the vector turnNotifications once when GenerateTurnSound is called.
   /// \param turn contains information about the next turn.
   /// \param distanceToTurnMeters is distance to the next turn in meters.
   /// \param turnNotifications is a parameter to fill it if it's necessary.
   /// \note The client implies turnNotifications does not contain empty strings.
  void GenerateTurnSound(vector<TurnItemDist> const & turns, vector<string> & turnNotifications);
  /// Reset states which reflects current route position.
  /// The method shall be called after creating a new route or after rerouting.
  void Reset();
};
}  // namespace sound
}  // namespace turns
}  // namespace routing
