#include "platform/localization.hpp"

#include "platform/settings.hpp"

#include <string>

namespace platform
{
namespace
{
enum class MeasurementType
{
  Distance,
  Speed,
  Altitude
};

LocalizedUnits const & GetLocalizedUnits(measurement_utils::Units units, MeasurementType measurementType)
{
  static LocalizedUnits const lengthImperial = {GetLocalizedString("ft"), GetLocalizedString("mi")};
  static LocalizedUnits const lengthMetric = {GetLocalizedString("m"), GetLocalizedString("km")};

  static LocalizedUnits const speedImperial = {GetLocalizedString("ft"), GetLocalizedString("miles_per_hour")};
  static LocalizedUnits const speedMetric = {GetLocalizedString("m"), GetLocalizedString("kilometers_per_hour")};

  switch (measurementType)
  {
  case MeasurementType::Distance:
  case MeasurementType::Altitude:
    switch (units)
    {
    case measurement_utils::Units::Imperial: return lengthImperial;
    case measurement_utils::Units::Metric: return lengthMetric;
    }
    break;
  case MeasurementType::Speed:
    switch (units)
    {
    case measurement_utils::Units::Imperial: return speedImperial;
    case measurement_utils::Units::Metric: return speedMetric;
    }
  }
  UNREACHABLE();
}
}  // namespace

LocalizedUnits const & GetLocalizedDistanceUnits()
{
  return GetLocalizedUnits(measurement_utils::GetMeasurementUnits(), MeasurementType::Distance);
}

LocalizedUnits const & GetLocalizedAltitudeUnits()
{
  return GetLocalizedUnits(measurement_utils::GetMeasurementUnits(), MeasurementType::Altitude);
}

std::string const & GetLocalizedSpeedUnits(measurement_utils::Units units)
{
  return GetLocalizedUnits(units, MeasurementType::Speed).m_high;
}

std::string const & GetLocalizedSpeedUnits()
{
  return GetLocalizedSpeedUnits(measurement_utils::GetMeasurementUnits());
}
}  // namespace platform
