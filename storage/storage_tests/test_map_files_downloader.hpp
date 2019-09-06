#pragma once

#include "storage/http_map_files_downloader.hpp"

namespace storage
{
class TestMapFilesDownloader : public HttpMapFilesDownloader
{
private:
  // MapFilesDownloader overrides:
  void GetServersList(ServersListCallback const & callback) override;
};
}  // namespace storage
