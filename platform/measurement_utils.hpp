#pragma once

#include "geometry/point2d.hpp"

#include <optional>
#include <string>

namespace measurement_utils
{
using OptionalStringRef = std::optional<std::string> const &;

enum class Units
{
  Metric = 0,
  Imperial = 1
};

std::string DebugPrint(Units units);

Units GetMeasurementUnits();

inline double MetersToMiles(double m) { return m * 0.000621371192; }
inline double MilesToMeters(double mi) { return mi * 1609.344; }
inline double MiphToKmph(double miph) { return MilesToMeters(miph) / 1000.0; }
inline double KmphToMiph(double kmph) { return MetersToMiles(kmph * 1000.0); }
inline double MpsToKmph(double mps) { return mps * 3.6; }
inline double MetersToFeet(double m) { return m * 3.2808399; }
inline double FeetToMeters(double ft) { return ft * 0.3048; }
inline double FeetToMiles(double ft) { return ft * 5280; }
inline double InchesToMeters(double in) { return in / 39.370; }
inline double NauticalMilesToMeters(double nmi) { return nmi * 1852; }
inline double KmphToMps(double kmph) { return kmph * 1000 / 3600; }

double ToSpeedKmPH(double speed, Units units);
double MpsToUnits(double mps, Units units);

/// Takes into an account user settings [metric, imperial]
/// @param[in] m meters
/// @param[out] res formatted std::string for search
/// @return should be direction arrow drawed? false if distance is to small (< 1.0)
std::string FormatDistance(double distanceInMeters);
std::string FormatDistanceWithLocalization(double m, OptionalStringRef high, OptionalStringRef low);

/// @return Localized meters or feets string for altitude, depending on current measurement units.
std::string FormatAltitude(double altitudeInMeters);
std::string FormatAltitudeWithLocalization(double altitudeInMeters, OptionalStringRef localizedUnits);
/// @return Speed value string (without suffix) in km/h for Metric and in mph for Imperial.
std::string FormatSpeedNumeric(double metersPerSecond, Units units);

/// @param[in] dac  Digits after comma in seconds.
/// Use dac == 3 for our common conversions to DMS.
std::string FormatLatLonAsDMS(double lat, double lon, bool withComma, int dac = 3);
void FormatLatLonAsDMS(double lat, double lon, std::string & latText, std::string & lonText,
                       int dac = 3);
std::string FormatMercatorAsDMS(m2::PointD const & mercator, int dac = 3);
void FormatMercatorAsDMS(m2::PointD const & mercator, std::string & lat, std::string & lon,
                         int dac = 3);

/// Default dac == 6 for the simple decimal formatting.
std::string FormatLatLon(double lat, double lon, int dac = 6);
std::string FormatLatLon(double lat, double lon, bool withComma, int dac = 6);
void FormatLatLon(double lat, double lon, std::string & latText, std::string & lonText,
                  int dac = 6);
std::string FormatMercator(m2::PointD const & mercator, int dac = 6);
void FormatMercator(m2::PointD const & mercator, std::string & lat, std::string & lon, int dac = 6);

std::string FormatOsmLink(double lat, double lon, int zoom);

/// Converts OSM distance (height, ele etc.) to meters.
/// @returns false if fails.
bool OSMDistanceToMeters(std::string const & osmRawValue, double & outMeters);
/// Converts OSM distance (height, ele etc.) to meters std::string.
/// @returns empty std::string on failure.
std::string OSMDistanceToMetersString(std::string const & osmRawValue,
                                      bool supportZeroAndNegativeValues = true,
                                      int digitsAfterComma = 2);
}  // namespace measurement_utils
