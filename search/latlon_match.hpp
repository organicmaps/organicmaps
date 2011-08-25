#pragma once
#include "../base/base.hpp"
#include "../std/string.hpp"

namespace search
{

// Check if query can be represented as '(lat, lon)'.
bool MatchLatLon(string const & query, double & lat, double & lon,
                 double & precisionLat, double & precisionLon);

}  // namespace search
