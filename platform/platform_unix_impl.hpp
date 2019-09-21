#pragma once

#include <string>
#include <vector>

namespace pl
{
void EnumerateFilesByRegExp(std::string const & directory, std::string const & regexp,
                            std::vector<std::string> & res);
}  // namespace pl
