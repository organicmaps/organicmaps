#pragma once
#include "std/string.hpp"


namespace search
{

/// Parse input query for most input coordinates cases.
bool MatchLatLonDegree(string const & query, double & lat, double & lon);

}  // namespace search
