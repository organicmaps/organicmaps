#pragma once

#include <string>

namespace generator
{
void BuildIsolinesInfoSection(std::string const & isolinesPath, std::string const & country,
                              std::string const & mwmFile);
}  // namespace generator
