#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace routing
{
using CountryParentNameGetterFn = std::function<std::string(std::string const &)>;

bool BuildRoutingIndex(std::string const & filename, std::string const & country,
                       CountryParentNameGetterFn const & countryParentNameGetterFn);
bool BuildCrossMwmSection(std::string const & path, std::string const & mwmFile,
                          std::string const & country,
                          CountryParentNameGetterFn const & countryParentNameGetterFn,
                          std::string const & osmToFeatureFile, bool disableCrossMwmProgress);
}  // namespace routing
