#pragma once

#include "platform/servers_list.hpp"
#include "storage/map_files_downloader.hpp"

namespace storage
{
// Sorts the download pools by the ping results (the responsive endpoints by latency): the unanswered
// servers drop out of the maps pool, while the terrain pool keeps them last, since a mirror may lack
// some blocks. A pool without responsive servers keeps its original order.
void SortServersByPing(downloader::MetaConfig & metaConfig, downloader::MetaConfig::ServersList const & responsive);

class MapFilesDownloaderWithPing : public MapFilesDownloader
{
public:
  // MapFilesDownloader overrides:
  downloader::MetaConfig GetMetaConfig() override;
};
}  // namespace storage
