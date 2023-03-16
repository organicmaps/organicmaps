#pragma once

#include <string>
#include <vector>

namespace generator
{
namespace platform_helpers
{
std::vector<std::string> GetFullFilePathsByExt(std::string const & dir, std::string const & ext);

std::vector<std::string> GetFullDataTmpFilePaths(std::string const & dir);
}  // namespace platform_helpers
}  // namespace generator
