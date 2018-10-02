#include "generator/generator_tests/regions_common.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"

std::string GetFileName()
{
  auto & platform = GetPlatform();
  auto const tmpDir = platform.TmpDir();
  platform.SetWritableDirForTests(tmpDir);
  return base::JoinPath(tmpDir, "RegionInfoCollector.bin");
}
