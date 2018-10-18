#pragma once

#include <string>

namespace generator
{
namespace regions
{
bool GenerateRegions(std::string const & pathInRegionsTmpMwm,
                     std::string const & pathInRegionsCollector,
                     std::string const & pathOutRegionsKv,
                     std::string const & pathOutRepackedRegionsTmpMwm, bool verbose);
}  // namespace regions
}  // namespace generator
