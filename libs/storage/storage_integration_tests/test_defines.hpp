#pragma once

#include "storage/downloader.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/reader.hpp"

#include "defines.hpp"

#include <string>

namespace storage
{
extern std::string const kMapTestDir;
extern std::string const kTestWebServer;

// Use the catalog-free Storage testing constructor for map download fixtures.
inline std::string ReadCountriesJson()
{
  std::string json;
  GetPlatform().GetReader(COUNTRIES_FILE)->ReadAsString(json);
  return json;
}

// Common setup for every test that downloads a map: a clean writable directory and running
// Platform thread pools. The pools are needed because Storage::OnDownloadFinished validates a
// downloaded file on Platform::Thread::File, and Platform starts with them shut down.
// Declaration order matters - the pools are joined before the writable directory is wiped, so
// an in-flight validation task cannot race the removal.
class StorageTest
{
protected:
  WritableDirChanger const m_writableDirChanger{kMapTestDir};
  Platform::ThreadRunner m_threadRunner;
};
}  // namespace storage
