#pragma once

#include "geometry/latlon.hpp"

#include <optional>
#include <string>

namespace search
{
// Parses an input query as a British National Grid (OS Grid) reference, e.g. "SW 740 421".
std::optional<ms::LatLon> MatchOSGridCoords(std::string const & query);
}  // namespace search
