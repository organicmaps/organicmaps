#include "routing/turns_sound_settings.hpp"

#include "platform/measurement_utils.hpp"

#include "base/string_utils.hpp"


namespace routing
{
namespace turns
{
namespace sound
{
bool Settings::IsValid() const
{
  return m_lengthUnits != LengthUnits::Undefined &&
         m_minDistanceUnits <= m_maxDistanceUnits &&
         !m_soundedDistancesUnits.empty() &&
         is_sorted(m_soundedDistancesUnits.cbegin(), m_soundedDistancesUnits.cend());
}

double Settings::ComputeTurnDistance(double speedUnitsPerSecond) const
{
  ASSERT(IsValid(), ());

  double const turnNotificationDistance = m_timeSeconds * speedUnitsPerSecond;
  return my::clamp(turnNotificationDistance, m_minDistanceUnits, m_maxDistanceUnits);
}

uint32_t Settings::RoundByPresetSoundedDistancesUnits(uint32_t turnNotificationUnits) const
{
  ASSERT(IsValid(), ());

  auto it = upper_bound(m_soundedDistancesUnits.cbegin(), m_soundedDistancesUnits.cend(),
                        turnNotificationUnits, less_equal<uint32_t>());
  // Rounding up the result.
  if (it != m_soundedDistancesUnits.cend())
    return *it;

  ASSERT(false, ("m_soundedDistancesUnits shall contain bigger values."));
  return m_soundedDistancesUnits.empty() ? 0 : m_soundedDistancesUnits.back();
}

double Settings::ConvertMetersPerSecondToUnitsPerSecond(double speedInMetersPerSecond) const
{
  switch (m_lengthUnits)
  {
    case LengthUnits::Undefined:
      ASSERT(false, ());
      return 0.;
    case LengthUnits::Meters:
      return speedInMetersPerSecond;
    case LengthUnits::Feet:
      return MeasurementUtils::MetersToFeet(speedInMetersPerSecond);
  }

  // m_lengthUnits is equal to LengthUnits::Undefined or to unknown value.
  ASSERT(false, ("m_lengthUnits is equal to unknown value."));
  return 0.;
}

double Settings::ConvertUnitsToMeters(double distanceInUnits) const
{
  switch (m_lengthUnits)
  {
    case LengthUnits::Undefined:
      ASSERT(false, ());
      return 0.;
    case LengthUnits::Meters:
      return distanceInUnits;
    case LengthUnits::Feet:
      return MeasurementUtils::FeetToMeters(distanceInUnits);
  }

  // m_lengthUnits is equal to LengthUnits::Undefined or to unknown value.
  ASSERT(false, ());
  return 0.;
}

string DebugPrint(LengthUnits const & lengthUnits)
{
  switch (lengthUnits)
  {
    case LengthUnits::Undefined:
      return "LengthUnits::Undefined";
    case LengthUnits::Meters:
      return "LengthUnits::Meters";
    case LengthUnits::Feet:
      return "LengthUnits::Feet";
  }

  stringstream out;
  out << "Unknown LengthUnits value: " << static_cast<int>(lengthUnits);
  return out.str();
}

string DebugPrint(Notification const & notification)
{
  stringstream out;
  out << "Notification [ m_distanceUnits == " << notification.m_distanceUnits
      << ", m_exitNum == " << notification.m_exitNum
      << ", m_useThenInsteadOfDistance == " << notification.m_useThenInsteadOfDistance
      << ", m_turnDir == " << DebugPrint(notification.m_turnDir)
      << ", m_lengthUnits == " << DebugPrint(notification.m_lengthUnits) << " ]" << endl;
  return out.str();
}
}  // namespace sound
}  // namespace turns
}  // namespace routing
