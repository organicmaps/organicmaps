#pragma once

#include <string>

namespace generator
{
namespace regions
{
void GenerateRegions(std::string const & pathInRegionsTmpMwm,
                     std::string const & pathInRegionsCollector,
                     std::string const & pathOutRegionsKv,
                     std::string const & pathOutRepackedRegionsTmpMwm,
                     bool verbose,
                     size_t threadsCount = 1);
}  // namespace regions
}  // namespace generator
