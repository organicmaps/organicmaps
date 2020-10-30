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

LocalizedUnits GetLocalizedUnits(measurement_utils::Units units, MeasurementType measurementType)
{
  switch (measurementType)
  {
  case MeasurementType::Distance:
  case MeasurementType::Altitude:
    switch (units)
    {
    case measurement_utils::Units::Imperial: return {GetLocalizedString("foot"), GetLocalizedString("mile")};
    case measurement_utils::Units::Metric: return {GetLocalizedString("meter"), GetLocalizedString("kilometer")};
    }
  case MeasurementType::Speed:
    switch (units)
    {
    case measurement_utils::Units::Imperial: return {"", GetLocalizedString("miles_per_hour")};
    case measurement_utils::Units::Metric: return {"", GetLocalizedString("kilometers_per_hour")};
    }
  }
  UNREACHABLE();
}
}  // namespace

LocalizedUnits GetLocalizedDistanceUnits()
{
  auto units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);

  return GetLocalizedUnits(units, MeasurementType::Distance);
}

LocalizedUnits GetLocalizedAltitudeUnits()
{
  auto units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);

  return GetLocalizedUnits(units, MeasurementType::Altitude);
}

std::string GetLocalizedSpeedUnits()
{
  auto units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);

  return GetLocalizedUnits(units, MeasurementType::Speed).m_high;
}
}  // namespace platform
