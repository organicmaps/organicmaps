#include "platform/localization.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include <string>

namespace platform
{
namespace
{
enum class MeasurementType
{
  Distance = 0,
  Speed,
  Altitude
};

const LocalizedUnits & GetLocalizedUnits(measurement_utils::Units units, MeasurementType measurementType)
{
  static LocalizedUnits UnitsLenghImperial = {GetLocalizedString("foot"), GetLocalizedString("mile")};
  static LocalizedUnits UnitsLenghMetric = {GetLocalizedString("meter"), GetLocalizedString("kilometer")};

  static LocalizedUnits UnitsSpeedImperial = {GetLocalizedString("foot"), GetLocalizedString("miles_per_hour")};
  static LocalizedUnits UnitsSpeedMetric = {GetLocalizedString("meter"), GetLocalizedString("kilometers_per_hour")};

  switch (measurementType)
  {
  case MeasurementType::Distance:
  case MeasurementType::Altitude:
    switch (units)
    {
    case measurement_utils::Units::Imperial: return UnitsLenghImperial;
    case measurement_utils::Units::Metric: return UnitsLenghMetric;
    }
    break;
  case MeasurementType::Speed:
    switch (units)
    {
    case measurement_utils::Units::Imperial: return UnitsSpeedImperial;
    case measurement_utils::Units::Metric: return UnitsSpeedMetric;
    }
  }
  UNREACHABLE();
}
}  // namespace

LocalizedUnits GetLocalizedDistanceUnits()
{
  return GetLocalizedUnits(measurement_utils::GetMeasurementUnits(), MeasurementType::Distance);
}

LocalizedUnits GetLocalizedAltitudeUnits()
{
  return GetLocalizedUnits(measurement_utils::GetMeasurementUnits(), MeasurementType::Altitude);
}

const std::string & GetLocalizedSpeedUnits(measurement_utils::Units units)
{
  return GetLocalizedUnits(units, MeasurementType::Speed).m_high;
}

std::string GetLocalizedSpeedUnits()
{
  return GetLocalizedSpeedUnits(measurement_utils::GetMeasurementUnits());
}
}  // namespace platform
