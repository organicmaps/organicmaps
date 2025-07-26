#include "testing/testing.hpp"

#include "storage/map_files_downloader_with_ping.hpp"

#include "base/logging.hpp"

#include "private.h"

namespace downloader_tests
{

class DownloaderStub : public storage::MapFilesDownloaderWithPing
{
  virtual void Download(storage::QueuedCountry && queuedCountry) {}
};

UNIT_TEST(GetMetaConfig)
{
  if (std::string(METASERVER_URL).empty())
    return;

  base::ScopedLogLevelChanger logLevel(base::LDEBUG);
  Platform::ThreadRunner runner;

  DownloaderStub().GetMetaConfig([](downloader::MetaConfig const & metaConfig)
  {
    TEST_GREATER(metaConfig.m_serversList.size(), 0, ());
    for (auto const & s : metaConfig.m_serversList)
      LOG(LINFO, (s));
  });
}

}  // namespace downloader_tests
