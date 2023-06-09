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
  case Distance::Units::Kilometers: return Distance(distance / 1000, Distance::Units::Kilometers);
  case Distance::Units::Feet:
    return Distance(measurement_utils::MetersToFeet(distance), Distance::Units::Feet);
  case Distance::Units::Miles:
    return Distance(measurement_utils::MetersToMiles(distance), Distance::Units::Miles);
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
    return Distance(measurement_utils::FeetToMeters(distance), Distance::Units::Meters);
  case Distance::Units::Kilometers:
    return Distance(measurement_utils::FeetToMeters(distance) / 1000, Distance::Units::Kilometers);
  case Distance::Units::Miles:
    return Distance(measurement_utils::FeetToMiles(distance), Distance::Units::Miles);
  default: UNREACHABLE();
  }
}

Distance MilesTo(double distance, Distance::Units units)
{
  switch (units)
  {
  case Distance::Units::Meters:
    return Distance(measurement_utils::MilesToMeters(distance), Distance::Units::Meters);
  case Distance::Units::Kilometers:
    return Distance(measurement_utils::MilesToMeters(distance) / 1000, Distance::Units::Kilometers);
  case Distance::Units::Feet:
    return Distance(measurement_utils::MilesToFeet(distance), Distance::Units::Feet);
  default: UNREACHABLE();
  }
}

double WithPrecision(double value, uint8_t precision)
{
  double const factor = std::pow(10, precision);
  return std::round(value * factor) / factor;
}
}  // namespace

Distance::Distance() : Distance(0.0) {}

Distance::Distance(double distanceInMeters) : Distance(distanceInMeters, Units::Meters) {}

Distance::Distance(double distance, platform::Distance::Units units)
  : m_distance(distance), m_units(units)
{
}

Distance Distance::CreateFormatted(double distanceInMeters)
{
  return Distance(distanceInMeters).ToPlatformUnitsFormatted();
}

bool Distance::IsValid() const { return m_distance > std::numeric_limits<double>::epsilon(); }

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
  return To(measurement_utils::GetMeasurementUnits() == measurement_utils::Units::Metric
                ? Units::Meters
                : Units::Feet)
      .GetFormattedDistance();
}

double Distance::GetDistance() const { return m_distance; }

Distance::Units Distance::GetUnits() const { return m_units; }

std::string Distance::GetDistanceString() const
{
  std::ostringstream os;
  os << std::defaultfloat << m_distance;
  return os.str();
}

// LocalizedStrings for this case are supported only on iOS
#if defined(OMIM_OS_IPHONE)
std::string Distance::GetUnitsString() const
{
  switch (m_units)
  {
  case Units::Meters: return GetLocalizedString("meter");
  case Units::Kilometers: return GetLocalizedString("kilometer");
  case Units::Feet: return GetLocalizedString("foot");
  case Units::Miles: return GetLocalizedString("mile");
  default: UNREACHABLE();
  }
}
#else
std::string Distance::GetUnitsString() const
{
  switch (m_units)
  {
  case Units::Meters: return "m";
  case Units::Kilometers: return "km";
  case Units::Feet: return "ft";
  case Units::Miles: return "mi";
  default: UNREACHABLE();
  }
}
#endif

Distance Distance::GetFormattedDistance() const
{
  double constexpr roundingThreshold = 100.0;
  double constexpr highUnitThreshold = 1000.0;

  double newDistance = m_distance;
  Units newUnits = m_units;
  int precision = 0;

  if (newUnits == Units::Meters || newUnits == Units::Feet)
  {
    // Round distances over roundingThreshold units to roundingThreshold units, e.g. 112 -> 110,
    // 998 -> 1000
    if (newDistance > roundingThreshold)
      newDistance = round(newDistance / 10) * 10;

    if (newDistance >= highUnitThreshold)
    {
      switch (m_units)
      {
      case Units::Meters:
        newUnits = Units::Kilometers;
        newDistance /= 1000.0;
        break;
      case Units::Feet:
        newUnits = Units::Miles;
        newDistance /= 5280.0;
        break;
      default: UNREACHABLE();
      }
    }
    else
      precision = 1;

    // For distances of 10.0 high units and over round to a whole number, e.g. 9.98 -> 10,
    // 10.9 -> 11
    if (newDistance >= 10.0)
    {
      newDistance = round(newDistance);
      precision = 0;
    }
  }

  if (newUnits == Units::Kilometers || newUnits == Units::Miles)
  {
    // Round distances over roundingThreshold units to roundingThreshold units, e.g. 112 -> 110,
    // 998 -> 1000
    if (newDistance > roundingThreshold)
      newDistance = round(newDistance / 10) * 10;

    precision = newDistance > 10.0 ? 0 : 1;
  }

  return Distance(WithPrecision(newDistance, precision), newUnits);
}

std::string Distance::ToString() const { return GetDistanceString() + " " + GetUnitsString(); }

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
