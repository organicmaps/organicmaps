#pragma once

#include "storage/http_map_files_downloader.hpp"

namespace storage
{
class TestMapFilesDownloader : public HttpMapFilesDownloader
{
public:
  TestMapFilesDownloader();
};
}  // namespace storage
