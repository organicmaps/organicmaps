#pragma once
#include "geometry/latlon.hpp"

#include <optional>
#include <string>

namespace search
{
// Parses input query for UTM and MGRS coordinate formats.
std::optional<ms::LatLon> MatchUTMCoords(std::string const & query);
std::optional<ms::LatLon> MatchMGRSCoords(std::string const & query);
}  // namespace search
