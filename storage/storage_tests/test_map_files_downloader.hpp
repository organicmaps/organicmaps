#pragma once

#include "storage/http_map_files_downloader.hpp"

namespace storage
{
class TestMapFilesDownloader : public HttpMapFilesDownloader
{
public:
  // MapFilesDownloader overrides:
  void GetServersList(int64_t const mapVersion, string const & mapFileName,
                      TServersListCallback const & callback) override;
};
}  // namespace storage
