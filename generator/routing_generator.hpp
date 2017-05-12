#pragma once

#include <string>

namespace routing
{
/// @param[in]  baseDir   Full path to .mwm files directory.
/// @param[in]  countryName   Country name same with .mwm and .border file name.
/// @param[in]  osrmFile  Full path to .osrm file (all prepared osrm files should be there).
void BuildRoutingIndex(std::string const & baseDir, std::string const & countryName, std::string const & osrmFile);

/// @param[in]  baseDir      Full path to .mwm files directory.
/// @param[in]  countryName   Country name same with .mwm and .border file name.
/// @param[in]  osrmFile  Full path to .osrm file (all prepared osrm files should be there).
/// perform if it's emplty.
void BuildCrossRoutingIndex(std::string const & baseDir, std::string const & countryName,
                            std::string const & osrmFile);
}
