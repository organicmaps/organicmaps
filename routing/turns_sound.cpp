#include "routing/turns_sound.hpp"

#include "platform/location.hpp"

namespace
{
// To inform an end user about the next turn with the help of an voice information message
// an operation system needs:
// - to launch TTS subsystem;
// - to pronounce the message.
// So to inform the user in time it's necessary to start
// kStartBeforeSeconds before the time. It is used in the following way:
// we start playing voice notice in kStartBeforeSeconds * TurnsSound::m_speedMetersPerSecond
// meters before the turn (for the second voice notification).
// When kStartBeforeSeconds * TurnsSound::m_speedMetersPerSecond  is too small or too large
// we use kMinStartBeforeMeters or kMaxStartBeforeMeters correspondingly.
uint32_t const kStartBeforeSeconds = 5;
uint32_t const kMinStartBeforeMeters = 10;
uint32_t const kMaxStartBeforeMeters = 100;

uint32_t CalculateDistBeforeMeters(double m_speedMetersPerSecond)
{
  ASSERT_LESS_OR_EQUAL(0, m_speedMetersPerSecond, ());
  uint32_t const startBeforeMeters =
      static_cast<uint32_t>(m_speedMetersPerSecond * kStartBeforeSeconds);
  if (startBeforeMeters <= kMinStartBeforeMeters)
    return kMinStartBeforeMeters;
  if (startBeforeMeters >= kMaxStartBeforeMeters)
    return kMaxStartBeforeMeters;
  return startBeforeMeters;
}
}  // namespace

namespace routing
{
namespace turns
{
namespace sound
{
void TurnsSound::GetRouteFollowingInfo(location::FollowingInfo & info, TurnItem const & turn,
                                       double distanceToTurnMeters) const
{
  if (m_turnNotificationEnabled)
  {
    if (m_nextTurnIndex != turn.m_index)
    {
      m_nextTurnNotificationProgress = SoundNotificationProgress::NoNotificationsPronounced;
      m_nextTurnIndex = turn.m_index;
    }

    uint32_t const distanceToPronounceNotificationMeters = CalculateDistBeforeMeters(m_speedMetersPerSecond);

    if (m_nextTurnNotificationProgress == SoundNotificationProgress::NoNotificationsPronounced)
    {
      double const currentSpeedUntisPerSecond =
          m_settings.ConvertMetersPerSecondToUnitsPerSecond(m_speedMetersPerSecond);
      double const turnNotificationDistUnits =
          m_settings.ComputeTurnNotificationDistanceUnits(currentSpeedUntisPerSecond);
      uint32_t const turnNotificationDistMeters =
          m_settings.ConvertUnitsToMeters(turnNotificationDistUnits) + distanceToPronounceNotificationMeters;

      if (distanceToTurnMeters < turnNotificationDistMeters)
      {
        // First turn sound notification.
        uint32_t const distToPronounce =
            m_settings.RoundByPresetSoundedDistancesUnits(turnNotificationDistUnits);
        info.m_turnNotifications.emplace_back(distToPronounce, turn.m_exitNum, false, turn.m_turn,
                                              m_settings.GetLengthUnits());
        // @TODO(vbykoianko) Check if there's a turn immediately after the current turn.
        // If so add an extra item to info.m_turnNotifications with "then parameter".
        m_nextTurnNotificationProgress =
            SoundNotificationProgress::BeforehandNotificationPronounced;
      }
    }
    else if (m_nextTurnNotificationProgress ==
                 SoundNotificationProgress::BeforehandNotificationPronounced &&
             distanceToTurnMeters < distanceToPronounceNotificationMeters)
    {
      info.m_turnNotifications.emplace_back(0, turn.m_exitNum, false, turn.m_turn,
                                            m_settings.GetLengthUnits());

      // @TODO(vbykoianko) Check if there's a turn immediately after the current turn.
      // If so add an extra item to info.m_turnNotifications with "then parameter".
      m_nextTurnNotificationProgress = SoundNotificationProgress::SecondNotificationPronounced;
    }
  }
}

void TurnsSound::EnableTurnNotification(bool enable)
{
  if (enable)
    Reset();
  m_turnNotificationEnabled = enable;
}

void TurnsSound::AssignSettings(Settings const & newSettings)
{
  ASSERT(newSettings.IsValid(), ());
  m_settings = newSettings;
}

void TurnsSound::SetSpeedMetersPerSecond(double speed)
{
  Reset();
  // When the quality of GPS data is bad the current speed may be less then zero.
  // It's easy to reproduce at an office with Nexus 5.
  // In that case zero meters per second is used.
  m_speedMetersPerSecond = max(0., speed);
}

void TurnsSound::Reset()
{
  m_nextTurnNotificationProgress =
      turns::sound::SoundNotificationProgress::NoNotificationsPronounced;
  m_nextTurnIndex = 0;
}

string DebugPrint(SoundNotificationProgress const notificationProgress)
{
  switch (notificationProgress)
  {
    case SoundNotificationProgress::NoNotificationsPronounced:
      return "NoNotificationsPronounced";
    case SoundNotificationProgress::BeforehandNotificationPronounced:
      return "BeforehandNotificationPronounced";
    case SoundNotificationProgress::SecondNotificationPronounced:
      return "SecondNotificationPronounced";
    default:
      ASSERT(false, ());
      stringstream out;
      out << "unknown SoundNotificationProgress (" << static_cast<int>(notificationProgress) << ")";
      return out.str();
  }
}
}  // namespace sound
}  // namespace turns
}  // namespace routing
