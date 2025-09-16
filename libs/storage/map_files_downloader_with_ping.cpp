#include "storage/map_files_downloader_with_ping.hpp"

#include "storage/pinger.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"

namespace storage
{
void MapFilesDownloaderWithPing::GetMetaConfig(MetaConfigCallback && callback)
{
  ASSERT(callback, ());

  downloader::MetaConfig metaConfig = LoadMetaConfig();
  CHECK(!metaConfig.servers.empty(), ());

  // Sort the list of servers by latency.
  auto const sorted = Pinger::ExcludeUnavailableAndSortEndpoints(metaConfig.servers);
  // Keep the original list if all servers are unavailable.
  if (!sorted.empty())
    metaConfig.servers = sorted;
  callback(std::move(metaConfig));
}
}  // namespace storage
