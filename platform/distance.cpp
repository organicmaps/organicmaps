#include "distance.hpp"

#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"

namespace platform
{
namespace
{
Distance MetersTo(double distance, Distance::Units units)
{
  switch (units)
  {
  case Distance::Units::Meters:
    return Distance(distance);
  case Distance::Units::Kilometers:
    return {distance / 1000, Distance::Units::Kilometers};
  case Distance::Units::Feet:
    return {measurement_utils::MetersToFeet(distance), Distance::Units::Feet};
  case Distance::Units::Miles:
    return {measurement_utils::MetersToMiles(distance), Distance::Units::Miles};
  default: UNREACHABLE();
  }
}

Distance KilometersTo(double distance, Distance::Units units)
{
  return MetersTo(distance * 1000, units);
}

Distance FeetTo(double distance, Distance::Units units)
{
  switch (units)
  {
  case Distance::Units::Meters:
    return {measurement_utils::FeetToMeters(distance), Distance::Units::Meters};
  case Distance::Units::Kilometers:
    return {measurement_utils::FeetToMeters(distance) / 1000, Distance::Units::Kilometers};
  case Distance::Units::Miles:
    return {measurement_utils::FeetToMiles(distance), Distance::Units::Miles};
  default: UNREACHABLE();
  }
}

Distance MilesTo(double distance, Distance::Units units)
{
  switch (units)
  {
  case Distance::Units::Meters:
    return {measurement_utils::MilesToMeters(distance), Distance::Units::Meters};
  case Distance::Units::Kilometers:
    return {measurement_utils::MilesToMeters(distance) / 1000, Distance::Units::Kilometers};
  case Distance::Units::Feet:
    return {measurement_utils::MilesToFeet(distance), Distance::Units::Feet};
  default: UNREACHABLE();
  }
}

double WithPrecision(double value, uint8_t precision)
{
  double const factor = std::pow(10, precision);
  return std::round(value * factor) / factor;
}
}  // namespace

Distance::Distance() : Distance(-1.0) {}

Distance::Distance(double distanceInMeters) : Distance(distanceInMeters, Units::Meters) {}

Distance::Distance(double distance, platform::Distance::Units units) : m_distance(distance), m_units(units) {}

Distance Distance::CreateFormatted(double distanceInMeters)
{
  return Distance(distanceInMeters).ToPlatformUnitsFormatted();
}

Distance Distance::CreateAltitudeFormatted(double meters)
{
  Distance elevation = Distance(meters).To(
      measurement_utils::GetMeasurementUnits() == measurement_utils::Units::Metric ? Units::Meters : Units::Feet);
  if (elevation.IsLowUnits())
    elevation.m_distance = WithPrecision(elevation.m_distance, 0);
  return elevation;
}

bool Distance::IsValid() const { return m_distance >= 0.0; }

bool Distance::IsLowUnits() const { return m_units == Units::Meters || m_units == Units::Feet; }

bool Distance::IsHighUnits() const { return !IsLowUnits(); }

Distance Distance::To(Distance::Units units) const
{
  if (m_units == units)
    return *this;

  switch (m_units)
  {
  case Units::Meters: return MetersTo(m_distance, units);
  case Units::Kilometers: return KilometersTo(m_distance, units);
  case Units::Feet: return FeetTo(m_distance, units);
  case Units::Miles: return MilesTo(m_distance, units);
  default: UNREACHABLE();
  }
}

Distance Distance::ToPlatformUnitsFormatted() const
{
  return To(measurement_utils::GetMeasurementUnits() == measurement_utils::Units::Metric ? Units::Meters : Units::Feet)
      .GetFormattedDistance();
}

double Distance::GetDistance() const
{
  return m_distance;
}

Distance::Units Distance::GetUnits() const { return m_units; }

std::string Distance::GetDistanceString() const
{
  if (!IsValid())
    return "";

  std::ostringstream os;
  os << std::defaultfloat << m_distance;
  return os.str();
}

std::string Distance::GetUnitsString() const
{
  switch (m_units)
  {
  case Units::Meters: return GetLocalizedString("m");
  case Units::Kilometers: return GetLocalizedString("km");
  case Units::Feet: return GetLocalizedString("ft");
  case Units::Miles: return GetLocalizedString("mi");
  default: UNREACHABLE();
  }
}

Distance Distance::GetFormattedDistance() const
{
  ASSERT(IsValid(), ());

  Distance newDistance = *this;
  int precision = 0;

  if (newDistance.m_units == Units::Kilometers)
    newDistance = newDistance.To(Units::Meters);
  else if (newDistance.m_units == Units::Miles)
    newDistance = newDistance.To(Units::Feet);

  double lowValue = round(newDistance.m_distance);
  // Round distances over 100 units to 10 units, e.g. 112 -> 110, 998 -> 1000
  if (lowValue > 100)
    lowValue = round(lowValue / 10) * 10;

  // Use high units for distances of 1000 units and over,
  // e.g. 1000m -> 1.0km, 1290m -> 1.3km, 1000ft -> 0.2mi
  if (lowValue >= 1000.0)
  {
    double highFactor = newDistance.m_units == Units::Meters ? 1000 : 5280;
    double highValue = newDistance.m_distance / highFactor;
    newDistance.m_distance = highValue;
    newDistance.m_units = newDistance.m_units == Units::Meters ? Units::Kilometers : Units::Miles;
    precision = round(highValue * 10) / 10 >= 10.0 ? 0 : 1;
  }
  else
    newDistance.m_distance = lowValue;

  if (IsHighUnits() && newDistance.GetUnits() != m_units)
    newDistance = *this;

  if (IsHighUnits() && newDistance.m_distance < 10)
    precision = 1;

  return {WithPrecision(newDistance.m_distance, precision), newDistance.m_units};
}

std::string Distance::ToString() const
{
  if (!IsValid())
    return "";

  return GetDistanceString() + " " + GetUnitsString();
}

std::string DebugPrint(Distance::Units units)
{
  switch (units)
  {
  case Distance::Units::Meters: return "Distance::Units::Meters";
  case Distance::Units::Kilometers: return "Distance::Units::Kilometers";
  case Distance::Units::Feet: return "Distance::Units::Feet";
  case Distance::Units::Miles: return "Distance::Units::Miles";
  default: UNREACHABLE();
  }
}
}  // namespace platform
