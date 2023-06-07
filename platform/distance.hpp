#pragma once

#include "platform/measurement_utils.hpp"

#include <string>

namespace platform
{
class Distance
{
public:
  enum class Units
  {
    Meters = 1,
    Kilometers = 2,
    Feet = 3,
    Miles = 4,

    // OSM-only
    NauticalMiles = 5,
    Inches = 6
  };

  Distance();

  explicit Distance(double distanceInMeters);

  explicit Distance(double distance, Units units);

  [[nodiscard]] static Distance CreateFormatted(double distanceInMeters);

  bool IsValid() const;

  [[nodiscard]] Distance To(Units units) const;
  [[nodiscard]] Distance ToPlatformUnitsFormatted() const;

  double GetDistance() const;
  Units GetUnits() const;

  [[nodiscard]] std::string GetDistanceString() const;
  [[nodiscard]] std::string GetUnitsString() const;

  /// Formats distance in the following way:
  /// * rounds distances over 100 units to 10 units, e.g. 112 -> 110, 998 -> 1000
  /// * for distances of 10.0 high units and over rounds to a whole number, e.g. 9.98 -> 10, 10.9 -> 11
  /// * use high units for distances of 1000 units and over, e.g. 1000m -> 1.0km, 1290m -> 1.3km, 1000ft -> 0.2mi
  [[nodiscard]] Distance GetFormattedDistance() const;

  [[nodiscard]] std::string ToString() const;

private:
  double m_distance;
  Units m_units;
};

std::string DebugPrint(Distance::Units units);
}  // namespace platform
