#pragma once

#include "routing/turns.hpp"

namespace routing
{
namespace turns
{
namespace sound
{
enum class LengthUnits
{
  Undefined,
  Meters,
  Feet
};

string DebugPrint(LengthUnits const & lengthUnits);

/// \brief The Settings struct is a structure for gathering information about turn sound
/// notifications settings.
/// All distance parameters shall be set in m_lengthUnits. (Meters of feet for the time being.)
class Settings
{
  uint32_t m_timeSeconds;
  uint32_t m_minDistanceUnits;
  uint32_t m_maxDistanceUnits;

  /// \brief m_distancesToPronounce is a list of distances in m_lengthUnits
  ///  which are ready to be pronounced.
  vector<uint32_t> m_soundedDistancesUnits;
  LengthUnits m_lengthUnits;

public:
  Settings()
      : m_minDistanceUnits(0),
        m_maxDistanceUnits(0),
        m_soundedDistancesUnits(),
        m_lengthUnits(LengthUnits::Undefined) {}
  Settings(uint32_t notificationTimeSeconds, uint32_t minNotificationDistanceUnits,
           uint32_t maxNotificationDistanceUnits, vector<uint32_t> const & soundedDistancesUnits,
           LengthUnits lengthUnits)
      : m_timeSeconds(notificationTimeSeconds),
        m_minDistanceUnits(minNotificationDistanceUnits),
        m_maxDistanceUnits(maxNotificationDistanceUnits),
        m_soundedDistancesUnits(soundedDistancesUnits),
        m_lengthUnits(lengthUnits)
  {
    ASSERT(!m_soundedDistancesUnits.empty(), ());
  }

  /// \brief IsValid checks if Settings data is consistent.
  /// \warning The complexity is up to linear in size of m_soundedDistancesUnits.
  bool IsValid() const;

  /// \brief computes the distance an end user shall be informed about the future turn
  /// before it, taking into account speedMetersPerSecond and fields of the structure.
  /// \param speedMetersPerSecond is a speed. For example it could be a current speed of an end
  /// user.
  /// \return distance in units which are set in m_lengthUnits. (Meters of feet for the time being.)
  double ComputeTurnDistance(double speedUnitsPerSecond) const;

  /// \brief RoundByPresetSoundedDistancesUnits rounds off its parameter by
  /// m_soundedDistancesUnits.
  /// \param turnNotificationDistance is a distance in m_lengthUnits.
  /// \return the distance which will be used (will be pronounced) in the next turn sound
  /// notification in m_lengthUnits units. (Meters of feet for the time being.)
  /// The result will be one of the m_soundedDistancesUnits values.
  uint32_t RoundByPresetSoundedDistancesUnits(uint32_t turnNotificationUnits) const;

  inline LengthUnits GetLengthUnits() const { return m_lengthUnits; }
  inline void SetLengthUnits(LengthUnits units) { m_lengthUnits = units; }
  double ConvertMetersPerSecondToUnitsPerSecond(double speedInMetersPerSecond) const;
  double ConvertUnitsToMeters(double distanceInUnits) const;
};

/// \brief The Notification struct contains all the information about the next sound
/// notification to pronounce.
struct Notification
{
  uint32_t m_distanceUnits;
  uint8_t m_exitNum;
  bool m_useThenInsteadOfDistance;
  TurnDirection m_turnDir;
  LengthUnits m_lengthUnits;

  Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
               TurnDirection turnDir, LengthUnits lengthUnits)
      : m_distanceUnits(distanceUnits),
        m_exitNum(exitNum),
        m_useThenInsteadOfDistance(useThenInsteadOfDistance),
        m_turnDir(turnDir),
        m_lengthUnits(lengthUnits)
  {
  }
  bool operator==(Notification const & rhv) const
  {
    return m_distanceUnits == rhv.m_distanceUnits && m_exitNum == rhv.m_exitNum &&
           m_useThenInsteadOfDistance == rhv.m_useThenInsteadOfDistance &&
           m_turnDir == rhv.m_turnDir && m_lengthUnits == rhv.m_lengthUnits;
  }

  inline bool IsValid() const { return m_lengthUnits != LengthUnits::Undefined; }
};

string DebugPrint(Notification const & turnGeom);

}  // namespace sound
}  // namespace turns
}  // namespace routing
