#include "testing/testing.hpp"

#include "storage/map_files_downloader_with_ping.hpp"

#include "base/logging.hpp"

#include "private.h"

namespace
{

class DownloaderStub : public storage::MapFilesDownloaderWithPing
{
  virtual void Download(storage::QueuedCountry && queuedCountry)
  {
  }
};

} // namespace

UNIT_TEST(GetServersList)
{
  if (std::string(METASERVER_URL).empty())
    return;

  base::ScopedLogLevelChanger logLevel(base::LDEBUG);
  Platform::ThreadRunner runner;

  DownloaderStub().GetServersList([](std::vector<std::string> const & vec)
  {
    TEST_GREATER(vec.size(), 0, ());
    for (auto const & s : vec)
      LOG(LINFO, (s));
  });
}
