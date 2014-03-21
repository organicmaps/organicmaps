#pragma once

#include "../std/string.hpp"

namespace MeasurementUtils
{

inline double MetersToMiles(double m) { return m * 0.000621371192; }
inline double MilesToMeters(double mi) { return mi * 1609.344; }
inline double MetersToYards(double m) { return m * 1.0936133; }
inline double YardsToMeters(double yd) { return yd * 0.9144; }
inline double MetersToFeet(double m) { return m * 3.2808399; }
inline double FeetToMeters(double ft) {  return ft * 0.3048; }
inline double FeetToMiles(double ft) { return ft * 5280; }
inline double YardToMiles(double yd) { return yd * 1760; }

/// Takes into an account user settings [metric, imperial]
/// @param[in] m meters
/// @param[out] res formatted string for search
/// @return should be direction arrow drawed? false if distance is to small (< 1.0)
bool FormatDistance(double m, string & res);

/// @param[in] dac  Digits after comma in seconds.
/// Use dac == 3 for our common conversions.
string FormatLatLonAsDMS(double lat, double lon, int dac = 0);

}
