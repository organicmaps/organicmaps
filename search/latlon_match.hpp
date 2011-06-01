#pragma once
#include "../base/base.hpp"
#include "../std/string.hpp"

namespace search
{

bool MatchLatLon(string const & s, double & lat, double & lon,
                 double & precisionLat, double & precisionLon);

}  // namespace search
