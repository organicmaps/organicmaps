#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/set.hpp"

namespace routing
{
/// @param[in]  baseDir   Full path to .mwm files directory.
/// @param[in]  countryName   Country name same with .mwm and .border file name.
/// @param[in]  osrmFile  Full path to .osrm file (all prepared osrm files should be there).
void BuildRoutingIndex(string const & baseDir, string const & countryName, string const & osrmFile);

/// @param[in]  baseDir      Full path to .mwm files directory.
/// @param[in]  countryName   Country name same with .mwm and .border file name.
/// @param[in]  osrmFile  Full path to .osrm file (all prepared osrm files should be there).
/// perform if it's emplty.
void BuildCrossRoutingIndex(string const & baseDir, string const & countryName,
                            string const & osrmFile);
}
