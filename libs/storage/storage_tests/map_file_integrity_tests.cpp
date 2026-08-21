#include "testing/testing.hpp"

#include "storage/storage_helpers.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/blake3.hpp"

#include "base/logging.hpp"

namespace storage
{
namespace
{
using platform::tests_support::ScopedFile;

UNIT_TEST(MapFileIntegrity_AcceptsMatchingHash)
{
  ScopedFile file("valid_download.mwm", "valid map data");
  auto const hash = coding::Blake3::CalculateMwmBase64(file.GetFullPath());

  TEST_EQUAL(ValidateDownloadedFile(file.GetFullPath(), hash), downloader::DownloadStatus::Completed, ());
  TEST(file.Exists(), ());
}

UNIT_TEST(MapFileIntegrity_DeletesMismatchedFile)
{
  base::ScopedLogAbortLevelChanger const ignoreErrors(LCRITICAL);
  ScopedFile file("invalid_download.mwm", "invalid map data");

  TEST_EQUAL(ValidateDownloadedFile(file.GetFullPath(), "wrong hash"), downloader::DownloadStatus::FailedIntegrityCheck,
             ());
  TEST(!file.Exists(), ());
  file.Reset();
}
}  // namespace
}  // namespace storage
