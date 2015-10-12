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

// kMinDistToSayNotificationMeters is minimum distance between two turns
// when pronouncing the first notification about the second turn makes sense.
uint32_t const kMinDistToSayNotificationMeters = 100;

uint32_t CalculateDistBeforeMeters(double m_speedMetersPerSecond)
{
  ASSERT_LESS_OR_EQUAL(0, m_speedMetersPerSecond, ());
  uint32_t const startBeforeMeters =
      static_cast<uint32_t>(m_speedMetersPerSecond * kStartBeforeSeconds);
  return my::clamp(startBeforeMeters, kMinStartBeforeMeters, kMaxStartBeforeMeters);
}

// Returns true if the closest turn is an entrance to a roundabout and the second is
// an exit form a roundabout.
// Note. There are some cases when another turn (besides an exit from roundabout)
// follows an entrance to a roundabout. It could happend in case of turns inside a roundabout.
// Returns false otherwise.
bool IsClassicEntranceToRoundabout(routing::turns::TurnItemDist const & firstTurn,
                                   routing::turns::TurnItemDist const & secondTurn)
{
  return firstTurn.m_turnItem.m_turn == routing::turns::TurnDirection::EnterRoundAbout
      && secondTurn.m_turnItem.m_turn == routing::turns::TurnDirection::LeaveRoundAbout;
}
}  // namespace

namespace routing
{
namespace turns
{
namespace sound
{
string TurnsSound::GenerateTurnText(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
                                    TurnDirection turnDir, ::Settings::Units lengthUnits) const
{
  Notification const notification(distanceUnits, exitNum, useThenInsteadOfDistance, turnDir, lengthUnits);
  return m_getTtsText(notification);
}

void TurnsSound::GenerateTurnSound(vector<TurnItemDist> const & turns, vector<string> & turnNotifications)
{
  turnNotifications.clear();

  if (!m_enabled || turns.empty())
    return;

  TurnItemDist const & firstTurn = turns.front();
  string firstNotification = GenerateFirstTurnSound(firstTurn.m_turnItem, firstTurn.m_distMeters);
  if (m_nextTurnNotificationProgress == PronouncedNotification::Nothing)
    return;
  if (firstNotification.empty())
    return;
  turnNotifications.emplace_back(move(firstNotification));

  // Generating notifications like "Then turn left" if necessary.
  if (turns.size() < 2)
    return;
  TurnItemDist const & secondTurn = turns[1];
  ASSERT_LESS_OR_EQUAL(firstTurn.m_distMeters, secondTurn.m_distMeters, ());
  if (secondTurn.m_distMeters - firstTurn.m_distMeters > kMaxTurnDistM
      && !IsClassicEntranceToRoundabout(firstTurn, secondTurn))
  {
    return;
  }
  string secondNotification = GenerateTurnText(0 /* distanceUnits is not used because of "Then" is used */,
                                               secondTurn.m_turnItem.m_exitNum, true,
                                               secondTurn.m_turnItem.m_turn,
                                               m_settings.GetLengthUnits());
  if (secondNotification.empty())
    return;
  turnNotifications.emplace_back(move(secondNotification));
  // Turn notification with word "Then" (about the second turn) will be pronounced.
  // When this second turn become the first one the first notification about the turn
  // shall be skipped.
  m_turnNotificationWithThen = true;
}

string TurnsSound::GenerateFirstTurnSound(TurnItem const & turn, double distanceToTurnMeters)
{
  if (m_nextTurnIndex != turn.m_index)
  {
    m_nextTurnNotificationProgress = PronouncedNotification::Nothing;
    m_nextTurnIndex = turn.m_index;
  }

  uint32_t const distanceToPronounceNotificationMeters = CalculateDistBeforeMeters(m_speedMetersPerSecond);

  if (m_nextTurnNotificationProgress == PronouncedNotification::Nothing)
  {
    if (distanceToTurnMeters > kMinDistToSayNotificationMeters)
    {
      double const currentSpeedUntisPerSecond =
          m_settings.ConvertMetersPerSecondToUnitsPerSecond(m_speedMetersPerSecond);
      double const turnNotificationDistUnits =
          m_settings.ComputeTurnDistance(currentSpeedUntisPerSecond);
      uint32_t const startPronounceDistMeters =
          m_settings.ConvertUnitsToMeters(turnNotificationDistUnits) + distanceToPronounceNotificationMeters;

      if (distanceToTurnMeters < startPronounceDistMeters)
      {
        if (m_turnNotificationWithThen)
        {
          FastForwardFirstTurnNotification();
        }
        else
        {
          double const distToPronounceMeters = distanceToTurnMeters - distanceToPronounceNotificationMeters;
          if (distToPronounceMeters < 0)
          {
            FastForwardFirstTurnNotification();
            return string(); // The current position is too close to the turn for the first notification.
          }

          // Pronouncing first turn sound notification.
          double const distToPronounceUnits = m_settings.ConvertMetersToUnits(distToPronounceMeters);
          uint32_t const roundedDistToPronounceUnits =
              m_settings.RoundByPresetSoundedDistancesUnits(distToPronounceUnits);
          m_nextTurnNotificationProgress = PronouncedNotification::First;
          return GenerateTurnText(roundedDistToPronounceUnits, turn.m_exitNum, false /* useThenInsteadOfDistance */,
                                  turn.m_turn, m_settings.GetLengthUnits());
        }
      }
    }
    else
    {
      // The first notification has not been pronounced but the distance to the turn is too short.
      // It happens if one turn follows shortly behind another one.
      m_nextTurnNotificationProgress = PronouncedNotification::First;
      FastForwardFirstTurnNotification();
    }
    return string();
  }

  if (m_nextTurnNotificationProgress == PronouncedNotification::First &&
      distanceToTurnMeters < distanceToPronounceNotificationMeters)
  {
    m_nextTurnNotificationProgress = PronouncedNotification::Second;
    FastForwardFirstTurnNotification();
    return GenerateTurnText(0 /* distanceUnits */, turn.m_exitNum,
                            false /* useThenInsteadOfDistance */,
                            turn.m_turn, m_settings.GetLengthUnits());
  }
  return string();
}

void TurnsSound::Enable(bool enable)
{
  if (enable && !m_enabled)
    Reset();
  m_enabled = enable;
}

void TurnsSound::SetLengthUnits(::Settings::Units units)
{
  m_settings.SetLengthUnits(units);
  switch(units)
  {
  case ::Settings::Metric:
    m_settings = Settings(30 /* notificationTimeSeconds */, 200 /* minNotificationDistanceUnits */,
                          2000 /* maxNotificationDistanceUnits */,
                          GetSoundedDistMeters() /* soundedDistancesUnits */,
                          ::Settings::Metric /* lengthUnits */);
    return;
  case ::Settings::Foot:
    m_settings = Settings(30 /* notificationTimeSeconds */, 500 /* minNotificationDistanceUnits */,
                          5000 /* maxNotificationDistanceUnits */,
                          GetSoundedDistFeet() /* soundedDistancesUnits */,
                          ::Settings::Foot /* lengthUnits */);
    return;
  }
}

void TurnsSound::SetSpeedMetersPerSecond(double speed)
{
  // When the quality of GPS data is bad the current speed may be less then zero.
  // It's easy to reproduce at an office with Nexus 5.
  // In that case zero meters per second is used.
  m_speedMetersPerSecond = max(0., speed);
}

void TurnsSound::Reset()
{
  m_nextTurnNotificationProgress = PronouncedNotification::Nothing;
  m_nextTurnIndex = 0;
  m_turnNotificationWithThen = false;
}

void TurnsSound::FastForwardFirstTurnNotification()
{
  m_turnNotificationWithThen = false;
  if (m_nextTurnNotificationProgress == PronouncedNotification::Nothing)
    m_nextTurnNotificationProgress = PronouncedNotification::First;
}

string DebugPrint(PronouncedNotification const notificationProgress)
{
  switch (notificationProgress)
  {
    case PronouncedNotification::Nothing:
      return "Nothing";
    case PronouncedNotification::First:
      return "First";
    case PronouncedNotification::Second:
      return "Second";
  }

  ASSERT(false, ());
  stringstream out;
  out << "unknown PronouncedNotification (" << static_cast<int>(notificationProgress) << ")";
  return out.str();
}
}  // namespace sound
}  // namespace turns
}  // namespace routing
