#pragma once

#include "platform/servers_list.hpp"
#include "storage/map_files_downloader.hpp"

namespace storage
{
class MapFilesDownloaderWithPing : public MapFilesDownloader
{
public:
  // MapFilesDownloader overrides:
  downloader::MetaConfig GetMetaConfig() override;
};
}  // namespace storage
