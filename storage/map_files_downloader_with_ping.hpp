#pragma once

#include "storage/map_files_downloader.hpp"

namespace storage
{
class MapFilesDownloaderWithPing : public MapFilesDownloader
{
private:
  void GetServersList(ServersListCallback const & callback) override;
};
}  // namespace storage
