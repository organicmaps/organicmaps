#pragma once

#include "storage/map_files_downloader.hpp"

namespace storage
{
class MapFilesDownloaderWithPing : public MapFilesDownloader
{
public:
  // MapFilesDownloader overrides:
  void GetMetaConfig(MetaConfigCallback const & callback) override;
};
}  // namespace storage
