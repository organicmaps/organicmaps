#include "storage/map_files_downloader_with_ping.hpp"

#include "storage/pinger.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <utility>

namespace storage
{
downloader::MetaConfig MapFilesDownloaderWithPing::GetMetaConfig()
{
  downloader::MetaConfig metaConfig = LoadMetaConfig();
  CHECK(!metaConfig.servers.empty(), ());

  // Ping shared mirrors once, then sort each download pool by latency.
  auto endpoints = metaConfig.servers;
  for (auto const & server : metaConfig.terrainServers)
    if (std::find(endpoints.begin(), endpoints.end(), server) == endpoints.end())
      endpoints.push_back(server);
  auto const sorted = Pinger::ExcludeUnavailableAndSortEndpoints(endpoints);
  auto const sortServers = [&sorted](auto & servers)
  {
    downloader::MetaConfig::ServersList available;
    for (auto const & server : sorted)
      if (std::find(servers.begin(), servers.end(), server) != servers.end())
        available.push_back(server);
    // Keep the original list if all its servers are unavailable.
    if (!available.empty())
      servers = std::move(available);
  };
  sortServers(metaConfig.servers);
  sortServers(metaConfig.terrainServers);

  return metaConfig;
}
}  // namespace storage
