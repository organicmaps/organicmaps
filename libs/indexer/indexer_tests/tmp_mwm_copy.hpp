#pragma once
#include "testing/testing.hpp"

#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

namespace tests
{
class TempMwmCopy
{
  DISALLOW_COPY_AND_MOVE(TempMwmCopy);

  std::string m_mapPath;

public:
  TempMwmCopy(std::string const & mwmName)
  {
    auto const originalMapPath = base::JoinPath(GetPlatform().ResourcesDir(), mwmName + DATA_FILE_EXTENSION);
    m_mapPath = base::JoinPath(GetPlatform().WritableDir(), mwmName + "-copy" + DATA_FILE_EXTENSION);
    base::CopyFileX(originalMapPath, m_mapPath);
  }

  std::string const & GetPath() const { return m_mapPath; }
  platform::LocalCountryFile GetLocalFile()
  {
    auto lcf = platform::LocalCountryFile::MakeTemporary(m_mapPath);
    lcf.SyncWithDisk();
    TEST(lcf.OnDisk(MapFileType::Map), ());
    return lcf;
  }

  ~TempMwmCopy() { base::DeleteFileX(m_mapPath); }
};
}  // namespace tests
