#pragma once
#include "std/string.hpp"


namespace search
{

/// Check if query can be represented as '(lat, lon)'.
/// @deprecated Use MatchLatLonDegree instead.
bool MatchLatLon(string const & query, double & lat, double & lon,
                 double & precisionLat, double & precisionLon);

/// Parse input query for most input coordinates cases.
bool MatchLatLonDegree(string const & query, double & lat, double & lon);

}  // namespace search
