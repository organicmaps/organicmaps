#include "storage/map_files_downloader_with_ping.hpp"

#include "storage/pinger.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <utility>

namespace storage
{
void SortServersByPing(downloader::MetaConfig & metaConfig, downloader::MetaConfig::ServersList const & responsive)
{
  auto const sortServers = [&responsive](auto & servers, bool keepUnavailable)
  {
    downloader::MetaConfig::ServersList available;
    for (auto const & server : responsive)
      if (std::find(servers.begin(), servers.end(), server) != servers.end())
        available.push_back(server);
    // Keep the original list if all its servers are unavailable.
    if (available.empty())
      return;
    if (keepUnavailable)
      for (auto const & server : servers)
        if (std::find(available.begin(), available.end(), server) == available.end())
          available.push_back(server);
    servers = std::move(available);
  };
  sortServers(metaConfig.servers, false /* keepUnavailable */);
  sortServers(metaConfig.terrainServers, true /* keepUnavailable */);
}

downloader::MetaConfig MapFilesDownloaderWithPing::GetMetaConfig()
{
  downloader::MetaConfig metaConfig = LoadMetaConfig();
  CHECK(!metaConfig.servers.empty(), ());

  // Ping shared mirrors once, then sort each download pool by latency.
  auto endpoints = metaConfig.servers;
  for (auto const & server : metaConfig.terrainServers)
    if (std::find(endpoints.begin(), endpoints.end(), server) == endpoints.end())
      endpoints.push_back(server);
  SortServersByPing(metaConfig, Pinger::ExcludeUnavailableAndSortEndpoints(endpoints));
  return metaConfig;
}
}  // namespace storage
