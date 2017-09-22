#pragma once

#include <string>

namespace routing
{
namespace transit
{
/// \brief Builds transit section at mwm.
/// \param mwmPath relative or full path to built mwm. The name of mwm without extension is considered
/// as country id.
/// \param transitDir a path to directory with json files with transit graphs.
void BuildTransit(std::string const & mwmPath, std::string const & transitDir);
}  // namespace transit
}  // namespace routing
