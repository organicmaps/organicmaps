#include "generator/platform_helpers.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "defines.hpp"

namespace generator
{
namespace platform_helpers
{
std::vector<std::string> GetFullFilePathsByExt(std::string const & dir, std::string const & ext)
{
  std::vector<std::string> result;
  Platform::GetFilesByExt(dir, ext, result);
  for (auto & p : result)
    p = base::JoinPath(dir, p);

  return result;
}

std::vector<std::string> GetFullDataTmpFilePaths(std::string const & dir)
{
  return GetFullFilePathsByExt(dir, DATA_FILE_EXTENSION_TMP);
}
}  // namespace platform_helpers
}  // namespace generator
