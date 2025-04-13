#pragma once

#include "platform/measurement_utils.hpp"

#include <string>

namespace platform
{
class Distance
{
public:
  /*!
   * \warning The order of values below shall not be changed.
   * \warning The values of Units shall be synchronized with values of Distance.Units enum in
   * java (see app.organicmaps.util.Distance for details).
   * \warning The values of Units shall be synchronized with values of unitLength func in
   * swift (see iphone/Maps/Classes/CarPlay/Templates Data/RouteInfo.swift for details).
   */
  enum class Units
  {
    Meters = 0,
    Kilometers = 1,
    Feet = 2,
    Miles = 3
  };

  Distance();

  explicit Distance(double distanceInMeters);

  Distance(double distance, Units units);

  static Distance CreateFormatted(double distanceInMeters);
  static std::string FormatAltitude(double meters);

  bool IsValid() const;

  bool IsLowUnits() const;
  bool IsHighUnits() const;

  Distance To(Units units) const;
  Distance ToPlatformUnitsFormatted() const;

  double GetDistance() const;
  Units GetUnits() const;

  std::string GetDistanceString() const;
  std::string GetUnitsString() const;

  /// Formats distance in the following way:
  /// * rounds distances over 100 units to 10 units, e.g. 112 -> 110, 998 -> 1000
  /// * for distances of 10.0 high units and over rounds to a whole number, e.g. 9.98 -> 10, 10.9 -> 11
  /// * use high units for distances of 1000 units and over, e.g. 1000m -> 1.0km, 1290m -> 1.3km, 1000ft -> 0.2mi
  Distance GetFormattedDistance() const;

  std::string ToString() const;

  friend std::string DebugPrint(Distance const & d) { return d.ToString(); }

private:
  double m_distance;
  Units m_units;
};

std::string DebugPrint(Distance::Units units);

}  // namespace platform
