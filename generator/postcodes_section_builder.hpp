#pragma once

#include <string>

namespace generator
{
bool BuildPostcodesSection(std::string const & path, std::string const & country,
                           std::string const & boundaryPostcodesFilename);
}  // namespace generator
