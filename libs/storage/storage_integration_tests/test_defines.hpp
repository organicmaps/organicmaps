#pragma once

#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"
#include "platform/settings.hpp"

#include <string>

namespace storage
{
extern std::string const kMapTestDir;
extern std::string const kTestWebServer;

// Common setup for every test that downloads a map: a clean writable directory and running
// Platform thread pools. The pools are needed because Storage::OnDownloadFinished validates a
// downloaded file on Platform::Thread::File, and Platform starts with them shut down.
// Declaration order matters - the pools are joined before the writable directory is wiped, so
// an in-flight validation task cannot race the removal.
// The download tests exercise the maps flow only: with the setting on, every fresh
// DownloadNode would also enqueue real terrain blocks (and hit the pre-scan ASSERT,
// since nothing runs the provider scan here). Restored on destruction.
class ScopedTerrainOff
{
public:
  ScopedTerrainOff()
  {
    m_had = settings::Get("TerrainWithMaps", m_old);
    settings::Set("TerrainWithMaps", false);
  }
  ~ScopedTerrainOff()
  {
    if (m_had)
      settings::Set("TerrainWithMaps", m_old);
    else
      settings::Delete("TerrainWithMaps");
  }

private:
  bool m_old = true;
  bool m_had = false;
};

class StorageTest
{
protected:
  WritableDirChanger const m_writableDirChanger{kMapTestDir};
  ScopedTerrainOff const m_terrainOff;
  Platform::ThreadRunner m_threadRunner;
};
}  // namespace storage
