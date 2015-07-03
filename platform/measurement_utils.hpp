#pragma once

#include "geometry/point2d.hpp"

#include "std/string.hpp"

namespace MeasurementUtils
{

inline double MetersToMiles(double m) { return m * 0.000621371192; }
inline double MilesToMeters(double mi) { return mi * 1609.344; }
inline double MetersToFeet(double m) { return m * 3.2808399; }
inline double FeetToMeters(double ft) {  return ft * 0.3048; }
inline double FeetToMiles(double ft) { return ft * 5280; }

/// Takes into an account user settings [metric, imperial]
/// @param[in] m meters
/// @param[out] res formatted string for search
/// @return should be direction arrow drawed? false if distance is to small (< 1.0)
bool FormatDistance(double m, string & res);

/// We always use meters and feet/yards for altitude
string FormatAltitude(double altitudeInMeters);
/// km/h or mph
string FormatSpeed(double metersPerSecond);

/// @param[in] dac  Digits after comma in seconds.
/// Use dac == 3 for our common conversions to DMS.
string FormatLatLonAsDMS(double lat, double lon, int dac = 3);
void FormatLatLonAsDMS(double lat, double lon, string & latText, string & lonText, int dac = 3);
string FormatMercatorAsDMS(m2::PointD const & mercator, int dac = 3);
void FormatMercatorAsDMS(m2::PointD const & mercator, string & lat, string & lon, int dac = 3);

/// Default dac == 6 for the simple decimal formatting.
string FormatLatLon(double lat, double lon, int dac = 6);
void FormatLatLon(double lat, double lon, string & latText, string & lonText, int dac = 6);
string FormatMercator(m2::PointD const & mercator, int dac = 6);
void FormatMercator(m2::PointD const & mercator, string & lat, string & lon, int dac = 6);

}
