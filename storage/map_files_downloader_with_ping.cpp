#include "storage/map_files_downloader_with_ping.hpp"

#include "storage/pinger.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"

namespace storage
{
void MapFilesDownloaderWithPing::GetServersList(ServersListCallback const & callback)
{
  ASSERT(callback , ());

  auto urls = LoadServersList();

  CHECK(!urls.empty(), ());

  auto const availableEndpoints = Pinger::ExcludeUnavailableEndpoints(urls);
  callback(availableEndpoints.empty() ? urls : availableEndpoints);
}
}  // namespace storage
