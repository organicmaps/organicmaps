#pragma once

#include "geometry/latlon.hpp"

#include <optional>
#include <string>

namespace search
{
// Parsers for the Irish coordinate systems (the island of Ireland). Unlike the OS Grid / UTM / MGRS
// matchers, an Irish Grid or ITM reference is easy to confuse with an ordinary query (a single grid
// letter, or a bare pair of numbers), so the processor treats these as "weak" matches: it shows the
// resulting coordinate at the top when the input is a valid in-range reference, but it ALSO runs the
// normal search, so a query that merely looks like one is never hijacked. Both return nullopt unless
// the input is a well-formed in-range reference; whether the decoded point is actually in Ireland is
// decided by the processor (by region), since these systems are offered only where they are official.

// Irish Grid, e.g. "O 152 345" (6-figure or finer).
std::optional<ms::LatLon> MatchIrishGridCoords(std::string const & query);

// Irish Transverse Mercator easting/northing, e.g. "715827 734693".
std::optional<ms::LatLon> MatchITMCoords(std::string const & query);
}  // namespace search
