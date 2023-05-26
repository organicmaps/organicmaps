#pragma once

#include <string>

namespace search
{
// Parses input query for UTM and MGRS coordinate formats.
bool MatchUTMCoords(std::string const & query, double & lat, double & lon);
bool MatchMGRSCoords(std::string const & query, double & lat, double & lon);
}  // namespace search
