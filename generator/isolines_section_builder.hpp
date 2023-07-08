#pragma once

#include <string>

namespace generator
{
// This section must be built with the same isolines file as had been used at the features stage.
void BuildIsolinesInfoSection(std::string const & isolinesPath, std::string const & country,
                              std::string const & mwmFile);
}  // namespace generator
