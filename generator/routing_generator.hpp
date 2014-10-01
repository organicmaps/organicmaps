#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace routing
{

/// @param[in]  mwmFile   Full path to .mwm file (.osm2ft file should be there).
/// @param[in]  osrmFile  Full path to .osrm file (all prepared osrm files should be there).
void BuildRoutingIndex(string const & mwmFile, string const & osrmFile);

}
