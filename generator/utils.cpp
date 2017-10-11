#include "generator/utils.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace generator
{
// SingleMwmIndex ---------------------------------------------------------------------------------
SingleMwmIndex::SingleMwmIndex(std::string const & mwmPath)
{
  m_countryFile = platform::LocalCountryFile::MakeTemporary(mwmPath);
  m_countryFile.SyncWithDisk();
  CHECK_EQUAL(
      m_countryFile.GetFiles(), MapOptions::MapWithCarRouting,
      ("No correct mwm corresponding local country file:", m_countryFile, ". Path:", mwmPath));

  auto const result = m_index.Register(m_countryFile);
  CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());
  CHECK(result.first.IsAlive(), ());
  m_mwmId = result.first;
}

void LoadIndex(Index & index)
{
  vector<platform::LocalCountryFile> localFiles;

  Platform & platform = GetPlatform();
  platform::FindAllLocalMapsInDirectoryAndCleanup(platform.WritableDir(), 0 /* version */,
                                                  -1 /* latestVersion */, localFiles);
  for (auto const & localFile : localFiles)
  {
    LOG(LINFO, ("Found mwm:", localFile));
    try
    {
      index.RegisterMap(localFile);
    }
    catch (RootException const & ex)
    {
      CHECK(false, (ex.Msg(), "Bad mwm file:", localFile));
    }
  }
}
}  // namespace generator
